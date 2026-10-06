#include <C2Core/time_core.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <thread>
#include <vector>

namespace C2Core::Time {

constexpr uint32_t FRAME_TIME_BUFFER_SIZE = 1000;

struct Context {
    uint64_t lastTimeNs = 0;

    double deltaTime = 0.0;
    float timeScale = 1.0f;

    double targetFPS = 0.0;
    double fixedDeltaTime = 0.0;
    double accumulator = 0.0;

    uint64_t spinThresholdNs = 2'000'000;

    uint32_t bufferIndex = 0;
    uint32_t sampleCount = 0;

    double frameTimes[FRAME_TIME_BUFFER_SIZE]{};
};

Context* create(double targetFps, double fixedUpdateRate) {
    Context* ctx = new Context();

    ctx->targetFPS = targetFps;

    if (fixedUpdateRate > 0.0) {
        ctx->fixedDeltaTime = 1.0 / fixedUpdateRate;
    }

    return ctx;
}

void destroy(Context* ctx) { delete ctx; }

void startFrame(Context* ctx) {
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count();

    if (ctx->lastTimeNs == 0) {
        ctx->lastTimeNs = nowNs;
    }

    ctx->deltaTime =
        (nowNs - ctx->lastTimeNs) / 1'000'000'000.0 * ctx->timeScale;

    ctx->accumulator += ctx->deltaTime;
    ctx->lastTimeNs = nowNs;
}

bool consumeFixedUpdate(Context* ctx) {
    if (ctx->fixedDeltaTime <= 0.0) {
        return false;
    }

    if (ctx->accumulator >= ctx->fixedDeltaTime) {
        ctx->accumulator -= ctx->fixedDeltaTime;
        return true;
    }

    return false;
}

void endFrame(Context* ctx, WaitMode mode) {
    uint64_t nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         std::chrono::steady_clock::now().time_since_epoch())
                         .count();

    uint64_t elapsedNs = nowNs - ctx->lastTimeNs;

    // targetFPS <= 0 means unlimited FPS.
    // We skip waiting, but still collect frame statistics.
    if (ctx->targetFPS > 0.0) {
        uint64_t targetFrameTimeNs =
            static_cast<uint64_t>(1'000'000'000.0 / ctx->targetFPS);

        if (elapsedNs < targetFrameTimeNs) {
            switch (mode) {
                case WaitMode::Sleep: {
                    uint64_t timeLeftNs = targetFrameTimeNs - elapsedNs;

                    std::this_thread::sleep_for(
                        std::chrono::nanoseconds(timeLeftNs));

                    break;
                }

                case WaitMode::Spin: {
                    while (elapsedNs < targetFrameTimeNs) {
                        nowNs = std::chrono::duration_cast<
                                    std::chrono::nanoseconds>(
                                    std::chrono::steady_clock::now()
                                        .time_since_epoch())
                                    .count();

                        elapsedNs = nowNs - ctx->lastTimeNs;

                        std::this_thread::yield();
                    }

                    break;
                }

                case WaitMode::Hybrid: {
                    uint64_t timeLeftNs = targetFrameTimeNs - elapsedNs;

                    if (timeLeftNs > ctx->spinThresholdNs) {
                        std::this_thread::sleep_for(std::chrono::nanoseconds(
                            timeLeftNs - ctx->spinThresholdNs));
                    }

                    while (elapsedNs < targetFrameTimeNs) {
                        nowNs = std::chrono::duration_cast<
                                    std::chrono::nanoseconds>(
                                    std::chrono::steady_clock::now()
                                        .time_since_epoch())
                                    .count();

                        elapsedNs = nowNs - ctx->lastTimeNs;

                        std::this_thread::yield();
                    }

                    break;
                }
            }
        }
    }

    // Re-read the time because Sleep/Spin/Hybrid may have waited.
    nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count();

    double elapsedMs = (nowNs - ctx->lastTimeNs) / 1'000'000.0;

    ctx->frameTimes[ctx->bufferIndex] = elapsedMs;

    ctx->bufferIndex = (ctx->bufferIndex + 1) % FRAME_TIME_BUFFER_SIZE;

    if (ctx->sampleCount < FRAME_TIME_BUFFER_SIZE) {
        ++ctx->sampleCount;
    }
}

Stats getStats(const Context* ctx) {
    Stats stats{};

    if (ctx->sampleCount == 0) {
        return stats;
    }

    std::vector<double> times(ctx->frameTimes,
                              ctx->frameTimes + ctx->sampleCount);

    double frameSum = std::accumulate(times.begin(), times.end(), 0.0);

    double averageFrameTime = frameSum / static_cast<double>(ctx->sampleCount);

    // bufferIndex points to the NEXT write position,
    // so the previous element is the latest frame.
    uint32_t latestIndex = (ctx->bufferIndex + FRAME_TIME_BUFFER_SIZE - 1) %
                           FRAME_TIME_BUFFER_SIZE;

    double latestFrameTime = ctx->frameTimes[latestIndex];

    stats.currentFps = latestFrameTime > 0.0 ? 1000.0 / latestFrameTime : 0.0;

    stats.averageFrameTime = averageFrameTime;

    // Sort from slowest frame to fastest frame.
    std::sort(times.begin(), times.end(), std::greater<double>());

    stats.maxFrameTime = times.front();
    stats.minFrameTime = times.back();

    uint32_t worstFrameCount =
        static_cast<uint32_t>(std::ceil(ctx->sampleCount * 0.01));

    worstFrameCount = std::max(1u, worstFrameCount);

    double worstFrameSum =
        std::accumulate(times.begin(), times.begin() + worstFrameCount, 0.0);

    double averageWorstFrameTime = worstFrameSum / worstFrameCount;

    stats.low1Percent =
        averageWorstFrameTime > 0.0 ? 1000.0 / averageWorstFrameTime : 0.0;

    return stats;
}

double getDeltaTime(const Context* ctx) { return ctx->deltaTime; }

void setTargetFPS(Context* ctx, double targetFps) {
    ctx->targetFPS = std::max(0.0, targetFps);
}

void setSpinThreshold(Context* ctx, uint64_t value) {
    ctx->spinThresholdNs = value;
}

}  // namespace C2Core::Time