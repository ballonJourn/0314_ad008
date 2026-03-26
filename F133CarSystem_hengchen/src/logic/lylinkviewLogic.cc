#pragma once
#include "uart/ProtocolSender.h"
#include "system/Mutex.h"
#include "utils/ScreenHelper.h"
#include "manager/ConfigManager.h"
#include "link/context.h"
#include "bt/context.h"
#include "system/setting.h"
#include "fy/os.hpp"
#include "common.h"
#include "media/audio_context.h"
#include "sysapp_context.h"
#include "system/reverse.h"
#include "utils/BrightnessHelper.h"
#include <base/ui_handler.h>
#include "utils/mem_profiler.h"
#include "utils/SlideManager.h"
#include "net/context.h"

#define DELAY_PLAY_TIMER     1
#define STOP_TIMER		9

#define FRAME_VIEW_SYNC      0
#define TOUCH_MOVE_THRESHOLD 5

static bt_cb_t _s_bt_cb;

typedef struct {
	const char *data;
	LYLINK_KEYCODE_E keycode;
} square_data;

static square_data _s_square_data_tab[] = {

	"HiCar200",LY_dpadActionRight,//hicar右移键值
	"HiCar100",LY_dpadActionLeft,//hicar左移键值
	"HiCar040",LY_knobActionSelect,//hicar选择键值
	"HiCar020",LY_knobActionHome,//hicar home键值
	"HiCar010",LY_knobActionBack,//hicar 返回键值

	"CpAuto200",LY_knobActionClockWise,//carplay,auto右移键值
	"CpAuto100",LY_knobActionCounterClockWise,//carplay,auto左移键值
	"CpAuto040",LY_knobActionSelect,//carplay,auto 选择键值
	"CpAuto020",LY_knobActionHome,//carplay,auto home键值
	"CpAuto010",LY_knobActionBack,//carplay,auto 返回键值

	"CpAuto0800",LY_knobActionClockWise,
	"CpAuto1000",LY_knobActionSelect,
	"CpAuto0400",LY_knobActionCounterClockWise,
};

/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	//{0,  6000}, //定时器id=0, 时间间隔6秒
	//{1,  1000},
};

static void _link_view_show(const LYLINKAPI_VIDEO_PARAM *param) {
	int w = param->width, h = param->height;
	int sw = LINK_VIEW_WIDTH, sh = LINK_VIEW_HEIGHT;
	int vx = 0, vy = 0, vw = sw, vh = sh;  // view pos
	int rot = CONFIGMANAGER->getScreenRotate();

	switch (lk::get_lylink_type()) {
	case LINK_TYPE_AIRPLAY:
	case LINK_TYPE_MIRACAST:
	case LINK_TYPE_WIFILY:
		if (w > 0 && h > 0) {
			if (w >= h) {
				// 手机横屏: 优先填满屏幕宽度，等比缩放高度
				vw = sw;
				vh = sw * h / w;
				if (vh > sh) {
					// 高度超出屏幕，则改为按高度适配
					vh = sh;
					vw = sh * w / h;
					vx = (sw - vw) / 2;
					vy = 0;
				} else {
					vx = 0;
					vy = (sh - vh) / 2;
				}
			} else {
				// 手机竖屏: 按高度适配，宽度居中
				vh = sh;
				vw = sh * w / h;
				if (vw > sw) {
					vw = sw;
					vh = sw * h / w;
					vy = (sh - vh) / 2;
					vx = 0;
				} else {
					vx = (sw - vw) / 2;
					vy = 0;
				}
			}
		}

		if ((rot == 90) || (rot == 270)) {
			std::swap(vx, vy);
			std::swap(vw, vh);
		}
		break;
//	case LINK_TYPE_WIFICP: {
//		vw = 1024
//		if ((rot == 90) || (rot == 270)) {
//			std::swap(vw, vh);
//		}
//	}
//		break;
	default:
		if ((rot == 90) || (rot == 270)) {
			std::swap(vw, vh);
		}
		break;
	}

	LOGD("[lylinkview] video_show: src(%d x %d) -> view(%d, %d, %d, %d) screen(%d x %d)",
		w, h, vx, vy, vw, vh, sw, sh);
	lk::video_show(vx, vy, vw, vh);
}

static void _link_view_hide() {
	lk::video_hide();
}

static void _link_touch(LYLINK_TOUCHMODE_E mode, int x, int y) {
	// 触摸坐标映射: 屏幕坐标(0~SCREEN_WIDTH/HEIGHT) -> 视频协商坐标(0~params.width/height)
	// CarPlay/AndroidAuto等协商分辨率可能与物理屏幕不同，需要等比缩放
	LYLINKAPI_VIDEO_PARAM param;
	if (lk::get_video_param(param) && param.width > 0 && param.height > 0) {
		int sw = LINK_VIEW_WIDTH;   // 物理屏幕宽 1600
		int sh = LINK_VIEW_HEIGHT;  // 物理屏幕高 600
		if (param.width != sw || param.height != sh) {
			x = x * param.width / sw;
			y = y * param.height / sh;
		}
	}
	lylinkapi_touch(0, mode, x, y);
}

static void _lylink_video_callback(LYLINKAPI_EVENT evt, int para0, void *para1) {
	if(evt == LYLINK_VIDEO_START) {
		const LYLINKAPI_VIDEO_PARAM *param = (const LYLINKAPI_VIDEO_PARAM *) para1;
		LOGD("[lylinkview] LYLINK_VIDEO_START %d %d\n", param->width, param->height);
		_link_view_show(param);
	} else if(evt == LYLINK_VIDEO_DATA){
#if FRAME_VIEW_SYNC
		mVideoView1Ptr->setVisible(true);
#endif
	}
}


static const char* _link_key_to_str(LYLINK_KEYCODE_E type){
	switch(type){
	case LY_knobActionClockWise:
	case LY_dpadActionRight:
		return "Right";
	case LY_knobActionSelect:
		return "Select";
	case LY_knobActionCounterClockWise:
	case LY_dpadActionLeft:
		return "Left";
	case LY_knobActionHome:
		return "Home";
	case LY_knobActionBack:
		return "Back";
	default:
		return "unknown";
	}
}

static void _square_data_cb(const char *data){
	char keydata[24] = {0};
	if(lk::get_lylink_type() == LINK_TYPE_WIFIHICAR){
		sprintf(keydata,"HiCar%s",data);
	}else if(lk::get_lylink_type() == LINK_TYPE_WIFICP||lk::get_lylink_type() == LINK_TYPE_WIFIAUTO){
		sprintf(keydata,"CpAuto%s",data);
	}
	for(size_t i = 0; i < TAB_SIZE(_s_square_data_tab);i++){
		if (strcmp(_s_square_data_tab[i].data, keydata) == 0) {
			LOGD("[lylinkview] control    %s",_link_key_to_str(_s_square_data_tab[i].keycode));
			lylinkapi_key(_s_square_data_tab[i].keycode, KeyMode_Down);
			lylinkapi_key(_s_square_data_tab[i].keycode, KeyMode_Up);
			break;
		}
	}
}

static void _bt_add_cb() {
	_s_bt_cb.square_data_cb = _square_data_cb;
	bt::add_cb(&_s_bt_cb);
}

static void _bt_remove_cb() {
	bt::remove_cb(&_s_bt_cb);
}

static void _link_start() {
	lk::add_lylink_callback(_lylink_video_callback);

	LYLINKAPI_VIDEO_PARAM param;
	if (lk::get_video_param(param)) {
		// 有视频信息直接显示
		_link_view_show(&param);
	} else {
		// 没有视频信息，启动下视频
		lk::video_start();
	}
	lylinkapi_cmd(LYLINK_AUDIO_LYLINK);
//	if(lk::get_lylink_type() == LINK_TYPE_WIFILY || lk::get_lylink_type() == LINK_TYPE_USBLY) {
//		audio::change_audio_type(E_AUDIO_TYPE_LYLINK_MUSIC);
//	}
	LOGE("[lylinkview] link start get_audio_type = %d", audio::get_audio_type());
}

static void _link_stop() {
	LOGD("[lylinkview] link stop");
	LYLINK_TYPE_E link_type = lk::get_lylink_type();
	if (!sys::reverse_does_enter_status() && BRIGHTNESSHELPER->isScreenOn()) {
		sys::setting::set_reverse_topbar_show(true);
		app::show_topbar();
	}
	// [FIX] 倒车时的视频流处理——按协议类型区分策略
	// CarPlay/AndroidAuto: video_stop() 安全，退出倒车后协议能自动重新推流
	// Aicast(LINK_TYPE_WIFILY): video_stop() 会发LYLINK_VIDEO_NATIVE并清除video_info，
	//   但Aicast协议在收到VIDEO_NATIVE后认为投屏已结束，后续video_start()无法恢复画面
	//   → 退出倒车后黑屏。改为只隐藏h264图层(video_hide)，配合camera buffer降级(2个)
	//   和startPreview 200ms延迟来避免内存竞争
	if (sys::reverse_does_enter_status()) {
		if (link_type == LINK_TYPE_WIFILY) {
			// Aicast倒车(从lylinkview界面直接进入倒车): 只隐藏图层，不停止视频流，保持协议连接
			// 退出倒车后 _link_start() → get_video_param()仍有效 → 直接video_show恢复
			_link_view_hide();
		} else {
			// 其他互联倒车: 可以安全停止
			lk::video_stop();
		}
	} else {
		// 非倒车场景(用户通过floatwnd home等方式主动离开投屏界面): 统一停止视频
		// [FIX-AICAST-HOME] 之前AiCast在此分支被跳过(不stop video), 导致h264解码流
		//   持续运行消耗内存。当用户从mainLogic进入倒车时, camera buffer与h264争夺
		//   仅剩的几MB内存 → OOM → 卡死。
		//   修复策略: 对AiCast关闭WiFi P2P连接, 这会:
		//   1. 中断P2P数据链路 → h264解码器因无数据自动停止 → 释放解码buffer内存
		//   2. lylink检测到连接断开 → 触发LYLINK_LINK_DISCONN回调 → 完整清理协议层
		//   3. LYLINK_LINK_DISCONN中bt::power_on()恢复蓝牙 → 系统状态干净
		//   用户需要重新发起AiCast投屏, 但保证了系统稳定性和倒车安全性。
		//   lylink服务本身不停止(不调stop_lylink), 保持就绪状态等待下次连接。
		lk::video_stop();
		if (link_type == LINK_TYPE_WIFILY) {
			LOGD("[lylinkview] AiCast home exit: closing WiFi P2P to release resources");
			net::change_mode(E_NET_MODE_NULL, true);
		}
	}
	lk::remove_lylink_callback(_lylink_video_callback);
	_link_view_hide();
}

#if 1
namespace {

class ViewTouchListener: public ZKBase::ITouchListener {
public:
	virtual void onTouchEvent(ZKBase *pBase, const MotionEvent &ev) {
		if (lk::get_lylink_type() == LINK_TYPE_MIRACAST) {
			return;
		}

		switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN:
			_link_touch(TouchMode_Down, ev.mX, ev.mY);
			mLastEv = ev;
			break;
		case MotionEvent::E_ACTION_MOVE:	// 触摸滑动
			if ((abs(ev.mX - mLastEv.mX) >= TOUCH_MOVE_THRESHOLD) ||
					(abs(ev.mY - mLastEv.mY) >= TOUCH_MOVE_THRESHOLD)) {
				mLastEv = ev;
				_link_touch(TouchMode_Move, ev.mX , ev.mY);
			}
			break;
		default:
			_link_touch(TouchMode_Up, ev.mX, ev.mY);
			break;
		}
	}

private:
	MotionEvent mLastEv;
};

}

static ViewTouchListener _s_view_touch_listener;
#endif
/**
 * 当界面构造时触发
 */
static void onUI_init() {
    MEM_LIFECYCLE("lylinkview", "init");
    MEM_VM_DETAIL("lylinkview_init_ENTRY");
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	_bt_add_cb();
	LayoutPosition pos = LayoutPosition(0, 0, ScreenHelper::getScreenWidth(), ScreenHelper::getScreenHeight());
    base::runInUiThreadUniqueDelayed("lylink_view_init", [pos](){
        mVideoView1Ptr->setPosition(pos);
        mActivityPtr->setPosition(pos);
    }, 100);
#if FRAME_VIEW_SYNC
	mVideoView1Ptr->setVisible(false);
#endif
	mVideoView1Ptr->setTouchListener(&_s_view_touch_listener);
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
	MEM_LIFECYCLE("lylinkview", "show");
	MEM_WARN_IF_LOW("lylinkview_show", 3000);
	// 推迟检测播放，处理倒车界面未完全退出，播放异常问题
	//usleep(1000*1000);
	mActivityPtr->registerUserTimer(DELAY_PLAY_TIMER, 900);
  	// 进入互联界面释放一下内存
//	system("echo 3 > /proc/sys/vm/drop_caches");
	fy::drop_caches();
	app::hide_topbar();
	sys::setting::set_reverse_topbar_show(false);

	// 投屏界面有floatwnd，禁止下拉出navibar
	SLIDEMANAGER->setCanSlide(false);

	LOGD("[lylinkview] assist_switch: %d", sys::setting::is_assist_switch_enabled());
	if (sys::setting::is_assist_switch_enabled()) {
		LOGD("[lylink floatwnd] show floatwnd ---------\n");
		app::show_floatwnd();
	}
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	MEM_LIFECYCLE("lylinkview", "hide");
	MEM_VM_DETAIL("lylinkview_hide_ENTRY");
	mActivityPtr->unregisterUserTimer(DELAY_PLAY_TIMER);
	_link_stop();

	if (sys::setting::is_assist_switch_enabled()) {
		app::hide_floatwnd();
	}

	// 退出投屏界面，恢复navibar下拉功能
	SLIDEMANAGER->setCanSlide(true);

	// [P1] 释放pagecache为后续Activity腾出空间
	fy::drop_caches();
	MEM_SNAP_SIMPLE("lylinkview_hide_EXIT");
}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	MEM_LIFECYCLE("lylinkview", "quit");
	MEM_VM_DETAIL("lylinkview_quit_ENTRY");
	// DELAY(100);
	_bt_remove_cb();
	mVideoView1Ptr->setTouchListener(NULL);

//	mActivityPtr->registerUserTimer(STOP_TIMER, 900); // 没起到作用这里

    // 添加：强制停止CarPlay连接尝试
//    if (lk::get_lylink_type() == LINK_TYPE_WIFICP) {
//    	lylinkapi_cmd(LYLINK_DISCONNECT_CARPLAY);
//        DELAY(100); // 等待断开完成
//    }

	_link_stop();

	// 退出投屏界面，恢复navibar下拉功能
	SLIDEMANAGER->setCanSlide(true);

	if (sys::setting::is_assist_switch_enabled()) {
		app::hide_floatwnd();
	}

	// 退出投屏界面，恢复navibar下拉功能
	SLIDEMANAGER->setCanSlide(true);

}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {

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
static bool onUI_Timer(int id){
	switch (id) {
	case DELAY_PLAY_TIMER:
		_link_start();
		// 只执行一次，停掉该定时器
		return false;
	case STOP_TIMER:
		_link_stop();
		// 只执行一次，停掉该定时器
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
static bool onlylinkviewActivityTouchEvent(const MotionEvent &ev) {
	LOGD("[LYLINKVIEW] on touch event-------");
	switch (ev.mActionStatus) {
	case MotionEvent::E_ACTION_DOWN:	//触摸按下
		//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
		break;
	case MotionEvent::E_ACTION_MOVE:	        //触摸滑动
		break;
	case MotionEvent::E_ACTION_UP:  //触摸抬起
		break;
	default:
		break;
	}
	return false;
}