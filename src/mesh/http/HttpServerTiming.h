#pragma once

#include "Throttle.h"
#include "UptimeClock.h"

class HttpServerTiming
{
  private:
    static constexpr uint32_t ACTIVE_THRESHOLD_MS = 5000;
    static constexpr uint32_t MEDIUM_THRESHOLD_MS = 30000;
    static constexpr uint32_t RESTART_DELAY_MS = 5000;
    static constexpr int32_t ACTIVE_INTERVAL_MS = 50;
    static constexpr int32_t MEDIUM_INTERVAL_MS = 200;
    static constexpr int32_t IDLE_INTERVAL_MS = 1000;

    uint32_t lastActivityTime = Time::getMillis();
    uint32_t restartAt = 0;

  public:
    void markActivity() { lastActivityTime = Time::getMillis(); }

    int32_t getPollingInterval() const
    {
        if (Throttle::isWithinTimespanMs(lastActivityTime, ACTIVE_THRESHOLD_MS))
            return ACTIVE_INTERVAL_MS;
        if (Throttle::isWithinTimespanMs(lastActivityTime, MEDIUM_THRESHOLD_MS))
            return MEDIUM_INTERVAL_MS;
        return IDLE_INTERVAL_MS;
    }

    void scheduleRestart() { restartAt = Time::timerEndsAtMillis(RESTART_DELAY_MS); }

    bool shouldRestart() const { return restartAt != 0 && Throttle::deadlinePassed(restartAt); }
};
