

void TEST_THREADENTRY(HEAP heap, size_t* randomSizes, size_t sizesCount, TestResult* result, uint32_t /*threadIndex*/)
{
    (void)heap;

    PerfTestGlobals& globals = PerfTestGlobals::get();

    std::vector<void*> ptrsStorage;
    ptrsStorage.resize(PerfTestGlobals::kAllocationsCount);
    memset(&ptrsStorage[0], 0, sizeof(void*) * ptrsStorage.size());

/*
#ifdef _WIN32
    HANDLE thread = GetCurrentThread();
    SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadIdealProcessor(thread, threadIndex);
#endif
*/

    ON_THREAD_START;

    // spin wait
    while (globals.canStart.load() == 0)
    {
    }

    auto begin = std::chrono::steady_clock::now();

    uint64_t opCount = 0;

    uint32_t idx = 0;
    uint8_t pattern = 0;

    for (size_t i = 0; i < PerfTestGlobals::kAllocationsCount; i++)
    {
        size_t bytesCount = randomSizes[idx % sizesCount];
        ptrsStorage[i] = MALLOC(bytesCount, 16);
        // memset(ptrsStorage[i], pattern, bytesCount);
        opCount++;
        idx++;
        pattern++;
    }

    const int iterationsCount = 300;
    for (int iteration = 0; iteration < iterationsCount; iteration++)
    {
        for (size_t i = 0; i < PerfTestGlobals::kAllocationsCount; i++)
        {
            FREE(ptrsStorage[i]);
            opCount++;

            size_t bytesCount = randomSizes[idx % sizesCount];
            ptrsStorage[i] = MALLOC(bytesCount, 16);
            // memset(ptrsStorage[i], pattern, bytesCount);
            pattern++;
            opCount++;
            idx++;
        }
    }

    for (size_t i = 0; i < PerfTestGlobals::kAllocationsCount; i++)
    {
        FREE(ptrsStorage[i]);
        opCount++;
    }

    auto end = std::chrono::steady_clock::now();

    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    long long ms = elapsedTime.count();

    result->opCount = opCount;
    result->timeMs = ms;

    ON_THREAD_FINISHED;
}

void TEST_ENTRYPOINT()
{
    PerfTestGlobals& globals = PerfTestGlobals::get();

    globals.canStart.store(0);

    HEAP heap = CREATE_HEAP;
    std::vector<std::thread> threads;
    threads.reserve(PerfTestGlobals::kThreadsCount);
    std::vector<TestResult> results;
    results.resize(PerfTestGlobals::kThreadsCount);

    // create threads
    for (uint32_t i = 0; i < PerfTestGlobals::kThreadsCount; i++)
    {
        threads.push_back(
            std::thread(TEST_THREADENTRY, heap, globals.randomSequence.data(), globals.randomSequence.size(), &results[i], i));
    }

    // sleep for 2 seconds (just in case)
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // run
    globals.canStart.store(1);

    for (auto& t : threads)
    {
        t.join();
    }

    globals.canStart.store(0);
    std::vector<double> opsSec;
    opsSec.resize(PerfTestGlobals::kThreadsCount);

    for (uint32_t i = 0; i < PerfTestGlobals::kThreadsCount; i++)
    {
        opsSec[i] = results[i].opCount * 1000.0 / results[i].timeMs;
    }

    double timeMin = FLT_MAX;
    double timeMax = -FLT_MAX;
    double opsMin = FLT_MAX;
    double opsMax = -FLT_MAX;

    double opsAverage = 0.0f;
    for (uint32_t i = 0; i < PerfTestGlobals::kThreadsCount; i++)
    {
        double opsPerSec = opsSec[i];
        opsAverage += opsPerSec;
        opsMin = std::min(opsMin, opsPerSec);
        opsMax = std::max(opsMax, opsPerSec);
        timeMin = std::min(results[i].timeMs / 1000.0, timeMin);
        timeMax = std::max(results[i].timeMs / 1000.0, timeMax);
    }
    opsAverage /= (double)PerfTestGlobals::kThreadsCount;

    const char* s = STRINGIFY(ALLOCATOR_TEST_NAME);

    printf("%s;%d;%3.0f;%3.0f;%3.0f;%3.2f;%3.2f\n", s, PerfTestGlobals::kThreadsCount, opsMin, opsMax, opsAverage, timeMin, timeMax);

    DESTROY_HEAP;
}
