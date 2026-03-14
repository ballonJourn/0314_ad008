/*
 * hardware.cpp
 *
 *  Created on: 2024年3月6日
 *      Author: guoxs
 */

#include "system/setting.h"
#include "fy/thread.hpp"
#include "hardware.h"
#include "common.h"
#include <base/timer.hpp>
#include "utils/ScreenHelper.h"

namespace sys {
namespace hw {

static Mutex _s_temp_mutex;
static std::set<temp_protect_cb> _s_temp_cb_set;
base::timer temp_protect_timer;

static int _read_temp() {
	char str[32] = {0};
	FILE *temp_value = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
	if (!temp_value) {
		return 0;
	}
	fread(str, 1, 8, temp_value);
	fclose(temp_value);
//	LOGD("str %s\n", str);
	return strtol(str, NULL, 10);
}

static void _temp_protect() {
	if (!sys::setting::is_temp_protect_enable()) return ;
	int temp_num = _read_temp();
	if (temp_num == 0) return ;
	Mutex::Autolock _l(_s_temp_mutex);
	SET_FOREACH(_s_temp_cb_set, temp_protect_cb, it) {
		(*it)(temp_num);
	}
}

static void* _det_thread(void *args) {
	LOGD("[hw] det thread start...\n");
	while (true) {
		DELAY(100);
		if (temp_protect_timer.elapsed() > 5000) {
			_temp_protect();
			temp_protect_timer.reset();
		}
	}

	return NULL;
}


void init() {
	fy::run_thread(_det_thread, NULL);
}

void add_temp_protect_cb(temp_protect_cb cb) {
	Mutex::Autolock _l(_s_temp_mutex);
	_s_temp_cb_set.insert(cb);
}
void remove_temp_protect_cb(temp_protect_cb cb) {
	Mutex::Autolock _l(_s_temp_mutex);
	_s_temp_cb_set.erase(cb);
}

// 获取屏的大小转为字符串
const char *getScreenResolution() {
    static const std::string res =
        std::to_string(ScreenHelper::getScreenWidth()) + "x" +
        std::to_string(ScreenHelper::getScreenHeight());
    return res.c_str();
}

}
}


