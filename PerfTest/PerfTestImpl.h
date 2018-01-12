
#define PASTER(x,y) x ## y
#define EVALUATOR(x,y)  PASTER(x,y)

#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)


#define TEST_ENTRYPOINT  EVALUATOR(DoTest_, ALLOCATOR_TEST_NAME)
#define TEST_THREADENTRY  EVALUATOR(ThreadFunc_, ALLOCATOR_TEST_NAME)


#include <thread>
#include <vector>
#include <chrono>
#include <stdint.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif


void TEST_THREADENTRY(HEAP heap, size_t* randomSizes, size_t sizesCount, Result* result, uint32_t threadIndex)
{
	const int allocationsCount = 1000000;
	std::vector<void*> ptrsStorage;
	ptrsStorage.resize(allocationsCount);
	memset(&ptrsStorage[0], 0, sizeof(void*) * ptrsStorage.size());

#ifdef _WIN32
	HANDLE thread = GetCurrentThread();
	SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);

	SetThreadIdealProcessor(thread, threadIndex);
#endif

	ON_THREAD_START;

	//spin wait
	while (canStart.load() == 0)
	{
	}

	auto begin = std::chrono::steady_clock::now();

	uint64_t opCount = 0;

	uint32_t idx = 0;
	uint8_t pattern = 0;

	for (size_t i = 0; i < allocationsCount; i++)
	{
		size_t bytesCount = randomSizes[idx % sizesCount];
		ptrsStorage[i] = MALLOC(bytesCount, 16);
		//memset(ptrsStorage[i], pattern, bytesCount);
		opCount++;
		idx++;
		pattern++;
	}

	const int iterationsCount = 300;
	for (int iteration = 0; iteration < iterationsCount; iteration++)
	{
		for (size_t i = 0; i < allocationsCount; i++)
		{
			FREE(ptrsStorage[i]);
			opCount++;

			size_t bytesCount = randomSizes[idx % sizesCount];
			ptrsStorage[i] = MALLOC(bytesCount, 16);
			//memset(ptrsStorage[i], pattern, bytesCount);
			pattern++;
			opCount++;
			idx++;
		}
	}

	for (size_t i = 0; i < allocationsCount; i++)
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




void TEST_ENTRYPOINT (uint32_t threadsCount, size_t* randomSizes, size_t sizesCount)
{
	canStart.store(0);
	HEAP heap = CREATE_HEAP;

	std::vector<std::thread> threads;
	std::vector<Result> results;
	results.resize(threadsCount);

	// create threads
	for (uint32_t i = 0; i < threadsCount; i++)
	{
		threads.push_back(std::thread(TEST_THREADENTRY, heap, randomSizes, sizesCount, &results[i], i));
	}

	//sleep for 2 seconds
	std::this_thread::sleep_for(std::chrono::seconds(2));

	//run!
	canStart.store(1);

	// wait all threads
	for (auto& t : threads)
	{
		t.join();
	}


	std::vector<double> opsSec;
	opsSec.resize(threadsCount);

	for (uint32_t i = 0; i < threadsCount; i++)
	{
		opsSec[i] = results[i].opCount * 1000.0 / results[i].timeMs;
	}

	double timeMin = FLT_MAX;
	double timeMax = -FLT_MAX;
	double opsMin = FLT_MAX;
	double opsMax = -FLT_MAX;

	double opsAverage = 0.0f;
	for (uint32_t i = 0; i < threadsCount; i++)
	{
		double opsPerSec = opsSec[i];
		opsAverage += opsPerSec;
		opsMin = std::min(opsMin, opsPerSec);
		opsMax = std::max(opsMax, opsPerSec);
		timeMin = std::min(results[i].timeMs / 1000.0, timeMin);
		timeMax = std::max(results[i].timeMs / 1000.0, timeMax);
	}
	opsAverage /= (double)threadsCount;


	const char* s = STRINGIFY(ALLOCATOR_TEST_NAME);

	printf("------ test time min : %3.2f, max : %3.2f seconds -----\n", timeMin, timeMax);

	printf("%s;%d;%3.2f;%3.2f;%3.2f\n", s, threadsCount, opsMin, opsMax, opsAverage);

	DESTROY_HEAP;
}





#undef TEST_ENTRYPOINT
#undef EVALUATOR
#undef PASTER