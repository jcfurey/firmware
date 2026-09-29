#pragma once

#include "PhoneAPI.h"
#include "concurrency/OSThread.h"
#include "mesh/http/HttpServerTiming.h"
#include <Arduino.h>
#include <functional>

#if !MESHTASTIC_EXCLUDE_WEBSERVER

void initWebServer();
void createSSLCert();

class WebServerThread : private concurrency::OSThread
{
  private:
    HttpServerTiming timing;

  public:
    WebServerThread();
    void scheduleRestart();
    void markActivity();

  protected:
    virtual int32_t runOnce() override;
    int32_t getAdaptiveInterval();
};

extern WebServerThread *webServerThread;

#else
// Stub implementations when web server is excluded
inline void initWebServer() {}
inline void createSSLCert() {}

class WebServerThread
{
  public:
    WebServerThread() {}
    void scheduleRestart() {}
    void markActivity() {}
};

inline WebServerThread *webServerThread = nullptr;

#endif
