#pragma once
#include "uart/ProtocolSender.h"

#include "app/SysAppFactory.h"
#include "sysapp_context.h"
#include "config.h"
#include "system/setting.h"
#include "utils/TimeHelper.h"
#include "sysapp_context.h"
#include "mode_observer.h"
#include "media/audio_context.h"
#include "bt/context.h"
#include "os/MountMonitor.h"
#include "uart/context.h"
#include "net/NetManager.h"
#include "ntp/ntp.h"
#include "sys/sysinfo.h"
#include "utils/mem_profiler.h"

#define WIFIMANAGER			NETMANAGER->getWifiManager()

//extern bool handle_main_page_back();
extern bool handle_main_page_back();
// extern bool _is_transitioning_from_activity;
extern void set_activity_transition_flag(bool flag);

REGISTER_SYSAPP(APP_TYPE_SYS_TOPBAR, topbarActivity);
static bt_cb_t _s_bt_cb;
static void set_status_btn_visble_pos(ZKButton* pButton, bool isVisible);
extern void fold_statusbar();

static void sys_time_(tm t ) {
	char buf[128] = {0};
	int hour = t.tm_hour;
	if (!sys::setting::is_time_format_24h()) {
		mtimeSystemTextViewPtr->setTextTr((hour >= 12) ? "PM" : "AM");
		hour = (hour > 12) ? (hour % 12) : hour;
	} else {
		mtimeSystemTextViewPtr->setText("");
	}

	sprintf(buf, "%02d:%02d", hour, t.tm_min);
	mtimeTextViewPtr->setText(buf);
}

static void _bt_connect_cb(bt_connect_state_e state) {
	switch (state) {
	case E_BT_CONNECT_STATE_IDLE:                // 未连接
		mbtButtonPtr->setSelected(true);
		break;
	case E_BT_CONNECT_STATE_CONNECTING:          // 连接中
		break;
	case E_BT_CONNECT_STATE_CONNECTED:           // 已连接
		mbtButtonPtr->setSelected(false);
		break;
	default:
		break;
	}
}

static void _bt_power_cb(bt_power_state_e state) {
	switch (state) {
	case E_BT_POWER_STATE_OFF:
		mbtButtonPtr->setVisible(false);
		break;
	case E_BT_POWER_STATE_CHANGING:
		break;
	case E_BT_POWER_STATE_ON:
		mbtButtonPtr->setVisible(true);
		break;
	default:
		break;
	}
}

static void _bt_add_cb() {
	_s_bt_cb.connect_cb = _bt_connect_cb;
	_s_bt_cb.power_cb = _bt_power_cb;
	bt::add_cb(&_s_bt_cb);
}

static void _bt_remove_cb() {
	bt::remove_cb(&_s_bt_cb);
}

static void fm_status_cb(bool status){
	if(status){
		set_status_btn_visble_pos(mfmButtonPtr, status);
	}else{
		set_status_btn_visble_pos(mfmButtonPtr, status);
	}

}

namespace {

class MainWifiListener : public WifiManager::IWifiListener {
public:
	virtual void handleWifiEnable(E_WIFI_ENABLE event, int args) {
		LOGD("MyWifiListener handleWifiEnable event: %d\n", event);
		switch (event) {
		case E_WIFI_ENABLE_ENABLE:
			break;
		case E_WIFI_ENABLE_DISABLE:
		case E_WIFI_ENABLE_DISABLEING:
			set_status_btn_visble_pos(mNetButtonPtr, false);
			break;
		case E_WIFI_ENABLE_ENABLEING:
		case E_WIFI_ENABLE_UNKNOW:
			break;
		}
	}
	virtual void handleWifiConnect(E_WIFI_CONNECT event, int args) {
		switch(event) {
		case E_WIFI_CONNECT_CONNECTED:
			set_status_btn_visble_pos(mNetButtonPtr, true);
			break;
		case E_WIFI_CONNECT_CONNECTING:
			break;
		case E_WIFI_CONNECT_DICONNECTING:
		case E_WIFI_CONNECT_DISCONNECT:
			set_status_btn_visble_pos(mNetButtonPtr, false);
			break;
		case E_WIFI_CONNECT_ERROR:
		case E_WIFI_CONNECT_UNKNOW:
			break;
		}
	}
};
class topMountListener : public MountMonitor::IMountListener {
protected:
	virtual void notify(int what, int status, const char *msg) {
		switch (status) {
		case MountMonitor::E_MOUNT_STATUS_MOUNTED: {
			LOGD("media mount path: %s\n", msg);
			if (strcmp(msg, "/mnt/extsd") == 0) {
				set_status_btn_visble_pos(msdButtonPtr, true);
			} else if (strcmp(msg, "/mnt/usb1") == 0) {
				set_status_btn_visble_pos(musb1ButtonPtr, true);
			} else if (strcmp(msg, "/mnt/usbotg") == 0) {
				set_status_btn_visble_pos(musbButtonPtr, true);
			}
			break;
		}

		case MountMonitor::E_MOUNT_STATUS_UNMOUNTING: {
			LOGD("media remove path: %s\n", msg);
			if (strcmp(msg, "/mnt/extsd") == 0) {
				set_status_btn_visble_pos(msdButtonPtr, false);
			} else if (strcmp(msg, "/mnt/usbotg") == 0 && !MOUNTMONITOR->isMounted("/mnt/usb1")) {
				set_status_btn_visble_pos(musbButtonPtr, false);
			} else if (strcmp(msg, "/mnt/usb1") == 0 && !MOUNTMONITOR->isMounted("/mnt/usbotg")) {
				set_status_btn_visble_pos(musb1ButtonPtr, false);
			}
			break;
		}
		}
	}
};

}
static topMountListener _mount_listener;
static MainWifiListener mainWifiListener;

static ZKButton** BtnArr[] = {&mNetButtonPtr, &mfmButtonPtr, &msdButtonPtr, &musbButtonPtr, &musb1ButtonPtr};

static void set_status_btn_visble_pos(ZKButton* pButton, bool isVisible) {
	pButton->setVisible(isVisible);

	const int RIGHT = 413 + 60; 	//图标右对齐边界
	const int INTERTVAL = 12; 		//图标对齐间隙

	int icon_left = RIGHT - (*BtnArr[TABLESIZE(BtnArr)-1])->getPosition().mWidth;		// 最后一个图标左边界

	for (size_t i=0; i<TABLESIZE(BtnArr); i++) {
		if ((*BtnArr[i])->isVisible()) {
			LayoutPosition pos = (*BtnArr[i])->getPosition();
			icon_left = icon_left - INTERTVAL - pos.mWidth;
			pos.mLeft = icon_left;
			(*BtnArr[i])->setPosition(pos);
		}
	}
}

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

static void showSysInfo() {
	struct sysinfo info;
	sysinfo(&info);

	long freeram = info.freeram;
	char str[20] = {0};
	sprintf(str, "内存:%ldkb", freeram / 1000);
	mTextView2Ptr->setText(str);
//	LOGD("[freeram] ----%ldkb-----", freeram / 1000);
}

static void Icon_Init() {
	set_status_btn_visble_pos(msdButtonPtr, MOUNTMONITOR->isMounted("/mnt/extsd"));
	set_status_btn_visble_pos(musb1ButtonPtr, MOUNTMONITOR->isMounted("/mnt/usb1"));
	set_status_btn_visble_pos(musbButtonPtr, MOUNTMONITOR->isMounted("/mnt/usbotg"));
	set_status_btn_visble_pos(mNetButtonPtr,WIFIMANAGER->isConnected());


	uart::fm_switch(sys::setting::get_fm_switch());
	uart::add_fm_state_cb(fm_status_cb);
	uart::query_fmswitch();
//	char str[20] = {0};
//	if (_read_temp() > 0) {
//		float temp_celsius = _read_temp() / 1000.0;
//		sprintf(str, "温度:%.2f", temp_celsius);
//		mTextView1Ptr->setText(str);
//	}

	mbtButtonPtr->setVisible(bt::is_on());
	mbtButtonPtr->setSelected(bt::get_connect_state() == E_BT_CONNECT_STATE_IDLE);

	mvolumButtonPtr->setSelected(audio::get_system_vol() > 0.1);
	mvolumTextViewPtr->setText((int)(audio::get_system_vol()*100));
	WIFIMANAGER->addWifiListener(&mainWifiListener);
	MOUNTMONITOR->addMountListener(&_mount_listener);
	_bt_add_cb();
}
/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{0,  1000}, //定时器id=0, 时间间隔1秒
	{1,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init() {
	MEM_LIFECYCLE("topbar", "init");
	MEM_SNAP_SIMPLE("topbar_init_ENTRY");
    //Tips :添加 UI初始化的显示代码到这里,如:mText1->setText("123");
	Icon_Init();
	sys_time_(*TimeHelper::getDateTime());
}

/**
 * 当切换到该界面时触发
 */
static void onUI_intent(const Intent *intentPtr) {
    if (intentPtr != NULL) {
        //TODO
    }
}

/*
 * 当界面显示时触发
 */
static void onUI_show() {
	MEM_LIFECYCLE("topbar", "show");

}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	MEM_LIFECYCLE("topbar", "hide");

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	MEM_LIFECYCLE("topbar", "quit");
	MEM_SNAP_SIMPLE("topbar_quit_ENTRY");
	_bt_remove_cb();
	WIFIMANAGER->removeWifiListener(&mainWifiListener);
	MOUNTMONITOR->removeMountListener(&_mount_listener);
	uart::remove_fm_state_cb(fm_status_cb);
}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {
    //串口数据回调接口
}

/**
 * 定时器触发函数
 * 不建议在此函数中写耗时操作，否则将影响UI刷新
 * 参数： id
 *         当前所触发定时器的id，与注册时的id相同
 * 返回值: true
 *             继续运行当前定时器
 *         false
 *             停止运行当前定时器
 */
static bool onUI_Timer(int id) {
	char bri[20] = {0};
	switch (id) {
	case 0: {	//内存监视
//		showSysInfo();
		sys_time_(*TimeHelper::getDateTime());
//		char str[20] = {0};
//		if (_read_temp() > 0) {
//			float temp_celsius = _read_temp() / 1000.0;
//			sprintf(str, "温度:%.2f", temp_celsius);
//			LOGD("[read_temp] temp_celsius = %.2f", temp_celsius);
//			mTextView1Ptr->setText(str);
//		}
//		sprintf(bri, "背光:%d", sys::setting::get_brightness());
//		mTextView3Ptr->setText(bri);
		mvolumTextViewPtr->setText((int)(audio::get_system_vol()*100));
		mvolumButtonPtr->setSelected(audio::get_system_vol() > 0.1);
	}
		break;
	case 1:
		if (sys::setting::get_fm_switch()) {
			uart::fm_frequency(sys::setting::get_fm_frequency());
		}
		return false;
		default:
			break;
	}
    return true;
}

/**
 * 有新的触摸事件时触发
 * 参数：ev
 *         新的触摸事件
 * 返回值：true
 *            表示该触摸事件在此被拦截，系统不再将此触摸事件传递到控件上
 *         false
 *            触摸事件将继续传递到控件上
 */
static bool ontopbarActivityTouchEvent(const MotionEvent &ev) {
	LayoutPosition pos = EASYUICONTEXT->getNaviBar()->getPosition();
	if (pos.mTop != -pos.mHeight) {	return false; } 	// 导航栏下滑了
	switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
		default:
			break;
	}
	return false;
}

static bool onButtonClick_btButton(ZKButton *pButton) {
    LOGD(" ButtonClick btButton !!!\n");
    return false;
}
static bool onButtonClick_volumButton(ZKButton *pButton) {
    LOGD(" ButtonClick volumButton !!!\n");
	if (app::is_show_ctrlbar() && (mode::get_event_mode() == E_EVENT_MODE_VOICE)) {
		app::hide_ctrlbar();
	} else {
		mode::set_event_mode(E_EVENT_MODE_VOICE);
		app::show_ctrlbar();
	}
	LOGD("[ctrlbar] volum mode::get_event_mode() = %d", mode::get_event_mode());
    return false;
}

static bool onButtonClick_lightButton(ZKButton *pButton) {
    LOGD(" ButtonClick lightButton !!!\n");
	if (app::is_show_ctrlbar() && (mode::get_event_mode() == E_EVENT_MODE_BRIGHTNESS)) {
		app::hide_ctrlbar();
	} else {
		mode::set_event_mode(E_EVENT_MODE_BRIGHTNESS);
		app::show_ctrlbar();
	}
	LOGD("[ctrlbar] light mode::get_event_mode() = %d", mode::get_event_mode());
    return false;
}

static bool onButtonClick_sys_home(ZKButton *pButton) {
    LOGD(" ButtonClick sys_home !!!\n");

    const char *app = EASYUICONTEXT->currentAppName();

    // **新增：检查当前是否在需要延时的Activity中**
    bool should_delay_transition = false;
    if (app) {
        if (strcmp(app, "videoActivity") == 0 ||
            strcmp(app, "musicActivity") == 0 ||
            strcmp(app, "PhotoAlbumActivity") == 0 ||
            strcmp(app, "btMusicActivity") == 0) {
            should_delay_transition = true;
            LOGD("[topbar] Home from %s - will use delayed transition", app);
        }
    }

    if (app && (strcmp(app, "main2Activity") == 0)) {
        // if (handle_main_page_back()) {
        //     LOGD("Home button: returned to main page (page 0) in mainActivity");
        // } else {
        //     LOGD("Home button: already on main page (page 0) in mainActivity");
        // }
    } else if (app && (strcmp(app, "mainActivity") == 0)) {
        // 当前已在mainActivity, 按Home回到第1页
        if (handle_main_page_back()) {
            LOGD("Home button: returned to page 0 in mainActivity");
        } else {
            LOGD("Home button: already on page 0 in mainActivity");
        }
    } else {
        // **新增：设置过渡标记**
//         if (should_delay_transition) {
// //            _is_transitioning_from_activity = true;
//             set_activity_transition_flag(true);
//         }
        
        EASYUICONTEXT->openActivity("mainActivity");
        LOGD("Home button: opened mainActivity");
        // if (handle_main_page_back()) {
        //     LOGD("Home button: returned to main page (page 0) in mainActivity");
        // } else {
        //     LOGD("Home button: already on main page (page 0) in mainActivity");
        // }
    }

    fold_statusbar();
    return false;
}

static bool onButtonClick_backButton(ZKButton *pButton) {
    LOGD(" ButtonClick backButton !!!\n");
    const char *app = EASYUICONTEXT->currentAppName();

    if (sys::setting::get_navibar_show()) {
        fold_statusbar();
        return false;
    }

    // **新增：检查是否从videoActivity或其他媒体Activity返回**
    bool should_delay_transition = false;
    if (app) {
        if (strcmp(app, "videoActivity") == 0 ||
            strcmp(app, "musicActivity") == 0 ||
            strcmp(app, "PhotoAlbumActivity") == 0 ||
            strcmp(app, "btMusicActivity") == 0) {
            should_delay_transition = true;
            LOGD("[topbar] Returning from %s - will use delayed transition", app);
        }
    }

    switch (mode::get_switch_mode()) {
        case E_SWITCH_MODE_NULL:
            if (app && (strcmp(app, "desktopActivity") == 0)) {
                break;
            }

            // 当在mainActivity的第2/3页时，按返回键先回到第1页
            if (app && (strcmp(app, "mainActivity") == 0)) {
                if (handle_main_page_back()) {
                    LOGD("Back button: returned to page 0 in mainActivity");
                    break;
                }
            }

//             // **新增：设置过渡标记**
            // if (should_delay_transition) {
            //    _is_transitioning_from_activity = true;
            //     set_activity_transition_flag(true);
            // }

    		EASYUICONTEXT->goBack();
    		break;
    	case E_SWITCH_MODE_GOBACK:
    		mode::set_event_mode(E_EVENT_MODE_GOBACK);
    		break;
    	}
    return false;
}

static bool onButtonClick_fmButton(ZKButton *pButton) {
    LOGD(" ButtonClick fmButton !!!\n");
    return false;
}

static bool onButtonClick_sdButton(ZKButton *pButton) {
    LOGD(" ButtonClick sdButton !!!\n");
    return false;
}

static bool onButtonClick_usbButton(ZKButton *pButton) {
    LOGD(" ButtonClick usbButton !!!\n");
    return false;
}
static bool onButtonClick_NetButton(ZKButton *pButton) {
    LOGD(" ButtonClick NetButton !!!\n");
    return false;
}
static bool onButtonClick_usb1Button(ZKButton *pButton) {
    LOGD(" ButtonClick usb1Button !!!\n");
    return false;
}
