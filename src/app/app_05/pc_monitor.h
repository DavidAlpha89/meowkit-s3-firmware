/**
 * @file pc_monitor.h
 * @author Mingo
 * @brief PC Monitor app for Mooncake
 * @version 0.1
 * @date 2025-08-05
 * @copyright Copyright (c) 2025
 */
#pragma once
#include <mooncake.h>
#include "../../MeowKit.h"
#include "pc_monitor_protocol.h"

using namespace mooncake;

namespace MOONCAKE::APPS
{
    /**
     * @brief PC Monitor app
     */
    class PCMonitor : public AppAbility {
    public:
        PCMonitor(DEVICES* device);
        void onOpen() override;
        void onRunning() override;
        void onClose() override;
    private:
        pc_monitor::Parser _parser;
        lv_obj_t* _previousScreen = nullptr;
        uint32_t _lastDataMs = 0;
        bool _hasData = false;
        bool _stale = false;
    };
}
