// isStaticFilePath() in src/mesh/http/ContentHelper.h confines HTTP file operations to /static.
// These cases prevent deletion of preference files, traversal, and embedded-NUL truncation while
// allowing nested assets. HttpServerTiming in src/mesh/http/HttpServerTiming.h preserves the
// five-second restart delay and adaptive polling across millis rollover. A wrapped deadline of
// zero must stay armed, and an inactive restart must never fire.
#include "TestUtil.h"
#include "UptimeClock.h"
#include "mesh/http/ContentHelper.h"
#include "mesh/http/HttpServerTiming.h"
#include <cstdlib>
#include <string>
#include <unity.h>

static void test_static_paths_allow_nested_assets()
{
    const char *paths[] = {"/static/index.html", "/static/index.html.gz", "/static/assets/icons/radio.svg",
                           "/static/.well-known/config.json", "/static/images/caf\xc3\xa9.png"};
    for (const auto path : paths)
        TEST_ASSERT_TRUE_MESSAGE(isStaticFilePath(path), path);
}

static void test_static_paths_reject_preferences_and_traversal()
{
    const char *paths[] = {"",
                           "/prefs/config.proto",
                           "static/index.html",
                           "/static-other/index.html",
                           "/static",
                           "/static/",
                           "/static/assets/",
                           "/static//index.html",
                           "/static/../prefs/config.proto",
                           "/static/assets/../../prefs/config.proto",
                           "/static/./index.html",
                           "/static/assets/../index.html",
                           "/static/assets/./index.html",
                           "/static/assets\\..\\prefs\\config.proto",
                           "/static/a\nfile"};
    for (const auto path : paths)
        TEST_ASSERT_FALSE_MESSAGE(isStaticFilePath(path), path);
}

static void test_static_paths_reject_embedded_null()
{
    const std::string path = std::string("/static/index.html") + '\0' + "/../prefs/config.proto";
    TEST_ASSERT_FALSE(isStaticFilePath(path));
}

static void test_restart_is_inactive_until_requested()
{
    HttpServerTiming timing;
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::setTestMillis(UINT32_MAX);
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::setTestMillis(0);
    TEST_ASSERT_FALSE(timing.shouldRestart());
}

static void test_restart_waits_five_seconds()
{
    HttpServerTiming timing;
    timing.scheduleRestart();
    Time::advanceTestMillis(4999);
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::advanceTestMillis(1);
    TEST_ASSERT_TRUE(timing.shouldRestart());
}

static void test_restart_waits_across_rollover()
{
    Time::setTestMillis(UINT32_MAX - 2000);
    HttpServerTiming timing;
    timing.scheduleRestart();
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::advanceTestMillis(4999);
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::advanceTestMillis(1);
    TEST_ASSERT_TRUE(timing.shouldRestart());
}

static void test_restart_wrapped_zero_deadline_stays_armed()
{
    Time::setTestMillis(UINT32_MAX - 4999);
    HttpServerTiming timing;
    timing.scheduleRestart();
    Time::advanceTestMillis(5000);
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::advanceTestMillis(1);
    TEST_ASSERT_TRUE(timing.shouldRestart());
}

static void test_restart_can_be_rescheduled()
{
    HttpServerTiming timing;
    timing.scheduleRestart();
    Time::advanceTestMillis(4000);
    timing.scheduleRestart();
    Time::advanceTestMillis(1000);
    TEST_ASSERT_FALSE(timing.shouldRestart());
    Time::advanceTestMillis(4000);
    TEST_ASSERT_TRUE(timing.shouldRestart());
}

static void test_polling_slows_when_idle_and_recovers_on_activity()
{
    HttpServerTiming timing;
    TEST_ASSERT_EQUAL_INT32(50, timing.getPollingInterval());
    Time::advanceTestMillis(4999);
    TEST_ASSERT_EQUAL_INT32(50, timing.getPollingInterval());
    Time::advanceTestMillis(1);
    TEST_ASSERT_EQUAL_INT32(200, timing.getPollingInterval());
    Time::advanceTestMillis(24999);
    TEST_ASSERT_EQUAL_INT32(200, timing.getPollingInterval());
    Time::advanceTestMillis(1);
    TEST_ASSERT_EQUAL_INT32(1000, timing.getPollingInterval());
    timing.markActivity();
    TEST_ASSERT_EQUAL_INT32(50, timing.getPollingInterval());
}

static void test_polling_thresholds_survive_rollover()
{
    Time::setTestMillis(UINT32_MAX - 1000);
    HttpServerTiming timing;
    Time::advanceTestMillis(4999);
    TEST_ASSERT_EQUAL_INT32(50, timing.getPollingInterval());
    Time::advanceTestMillis(1);
    TEST_ASSERT_EQUAL_INT32(200, timing.getPollingInterval());
    Time::advanceTestMillis(25000);
    TEST_ASSERT_EQUAL_INT32(1000, timing.getPollingInterval());
}

extern "C" {
// Required by Unity: PlatformIO's weak defaults do not link on MinGW (PE-COFF weak externals).
void setUp(void)
{
    Time::setTestMillis(100);
}
void tearDown(void)
{
    Time::useRealClock();
}

void setup()
{
    initializeTestEnvironment();
    UNITY_BEGIN();
    RUN_TEST(test_static_paths_allow_nested_assets);
    RUN_TEST(test_static_paths_reject_preferences_and_traversal);
    RUN_TEST(test_static_paths_reject_embedded_null);
    RUN_TEST(test_restart_is_inactive_until_requested);
    RUN_TEST(test_restart_waits_five_seconds);
    RUN_TEST(test_restart_waits_across_rollover);
    RUN_TEST(test_restart_wrapped_zero_deadline_stays_armed);
    RUN_TEST(test_restart_can_be_rescheduled);
    RUN_TEST(test_polling_slows_when_idle_and_recovers_on_activity);
    RUN_TEST(test_polling_thresholds_survive_rollover);
    exit(UNITY_END());
}

void loop() {}
}
