#pragma once
#include "uart/ProtocolSender.h"
#include "system/setting.h"
#include "misc/storage.h"
#include "common.h"
#include "utils/BrightnessHelper.h"
#include "link/context.h"
#include "bt/context.h"
#include "media/audio_context.h"
#include "media/audio_linein.h"
#include "media/music_player.h"
#include "sysapp_context.h"
#include "fy/os.hpp"
#include "system/hardware.h"
#include "utils/mem_profiler.h"

#define DELAY_START_PREVIEM_TIMER 1
#define FORCE_HIDE_TOPBAR_TIMER   2

static int _s_signal_check_count;
//extern void screenOn_event();
extern void fold_statusbar();

// 细化音量记录结构，支持三种音量类型
typedef struct {
	float system_vol;		// 系统音量
	float bt_call_vol;		// 蓝牙通话音量
	float lylink_call_vol;	// Lylink通话音量
} volume_record_t;

static volume_record_t vol_record = {0.0, 0.0, 0.0};

//static bool effect = false;
namespace {

class MyPictureCallback : public ZKCameraView::IPictureCallback {
private:
	virtual const char* onPictureSavePath() {
		LOGD("onPictureSavePath!");
		return "/tmp/cam.jpg";
	}
};

class MyErrorCodeCallback : public ZKCameraView::IErrorCodeCallback {
private:
	virtual void onErrorCode(int error) {
//		LOGD("@@ onErrorCode error: %d\n", error);
		if (error == E_CAMERA_STATUS_CODE_NO_SIGNAL) {
			if (_s_signal_check_count < 10) {   // 检测几次无信号才出提示
				_s_signal_check_count++;
			} else {
				mSignTextViewPtr->setVisible(true);
			}
		} else if (error == E_CAMERA_STATUS_CODE_HAS_SIGNAL) {
			_s_signal_check_count = 0;
			mSignTextViewPtr->setVisible(false);
		}
	}
};
struct cam_info_t {
	const char *name;
	int w;
	int h;
	int rate;
	bool di_enable;   // 使能硬件奇偶合并
	bool checked;
};

static cam_info_t _s_cam_info_tab[] = {
	{ "AHD 720P 25", 1280, 720, 25, false, false },
	{ "AHD 720P 30", 1280, 720, 30, false, false },
	{ "TVI 720P 25", 1280, 720, 24, false, false },
	{ "TVI 720P 30", 1280, 720, 29, false, false },
	{ "AHD 1080P 25", 1920, 1080, 25, false, false },
	{ "AHD 1080P 30", 1920, 1080, 30, false, false },
	{ "TVI 1080P 25", 1920, 1080, 24, false, false },
	{ "TVI 1080P 30", 1920, 1080, 29, false, false },
	{ "CVBS PAL 50", 960, 576, 50, true, false },
	{ "CVBS NTSC 60", 960, 480, 60, true, false },
	{ "DM5885 50", 720, 480, 50, false, false },   // 逐行
	{ "DM5885 49", 720, 480, 49, true, false },    // 隔行
};

}

static MyPictureCallback sMyPictureCallback;
static MyErrorCodeCallback sMyErrorCodeCallback;

static void _draw_reverse_line() {
	SZKPoint lt, rt, lb, rb;
	sys::setting::get_reverse_line_point(lt, rt, lb, rb);

	SZKPoint points[3];

	int h = lb.y - lt.y;  // 垂直高度, 限制不为0
	int gh = REVERSE_LINE_G_RATIO * h;
	int yh = REVERSE_LINE_Y_RATIO * h;

	mLinePainterPtr->setLineWidth(REVERSE_LINE_WIDTH);

	// draw left
	int w = lb.x - lt.x;
	float ratio = (float) w / h;

	points[0].x = lt.x + REVERSE_LINE_CORNER_LEN;
	points[0].y = lt.y;
	points[1].x = lt.x;
	points[1].y = lt.y;
	points[2].x = ratio * gh + lt.x;
	points[2].y = lt.y + gh;
	mLinePainterPtr->setSourceColor(REVERSE_LINE_G_COLOR);
	mLinePainterPtr->drawLines(points, 3);

	points[0].x = points[2].x + REVERSE_LINE_CORNER_LEN;
	points[0].y = points[2].y;
	points[1].x = points[2].x;
	points[1].y = points[2].y;
	points[2].x = ratio * (gh + yh) + lt.x;
	points[2].y = lt.y + (gh + yh);
	mLinePainterPtr->setSourceColor(REVERSE_LINE_Y_COLOR);
	mLinePainterPtr->drawLines(points, 3);

	points[0].x = points[2].x + REVERSE_LINE_CORNER_LEN;
	points[0].y = points[2].y;
	points[1].x = points[2].x;
	points[1].y = points[2].y;
	points[2].x = lb.x;
	points[2].y = lb.y;
	mLinePainterPtr->setSourceColor(REVERSE_LINE_R_COLOR);
	mLinePainterPtr->drawLines(points, 3);

	// draw right
	w = rb.x - rt.x;
	ratio = (float) w / h;

	points[0].x = rt.x - REVERSE_LINE_CORNER_LEN;
	points[0].y = rt.y;
	points[1].x = rt.x;
	points[1].y = rt.y;
	points[2].x = ratio * gh + rt.x;
	points[2].y = rt.y + gh;
	mLinePainterPtr->setSourceColor(REVERSE_LINE_G_COLOR);
	mLinePainterPtr->drawLines(points, 3);

	points[0].x = points[2].x - REVERSE_LINE_CORNER_LEN;
	points[0].y = points[2].y;
	points[1].x = points[2].x;
	points[1].y = points[2].y;
	points[2].x = ratio * (gh + yh) + rt.x;
	points[2].y = rt.y + (gh + yh);
	mLinePainterPtr->setSourceColor(REVERSE_LINE_Y_COLOR);
	mLinePainterPtr->drawLines(points, 3);

	points[0].x = points[2].x - REVERSE_LINE_CORNER_LEN;
	points[0].y = points[2].y;
	points[1].x = points[2].x;
	points[1].y = points[2].y;
	points[2].x = rb.x;
	points[2].y = rb.y;
	mLinePainterPtr->setSourceColor(REVERSE_LINE_R_COLOR);
	mLinePainterPtr->drawLines(points, 3);
}

typedef struct {
	audio_type_e type;			  // 音频类型
	bool playing;				  // 保存切换声音类型时的播放状态
	bool (*is_playing)();		  // 获取播放状态的方法
	void (*pause)();			  // 设置暂停状态的方法
	void (*resume)();			  // 设置恢复状态的方法
} audio_reverse_t;

static void _linein_pause() {
	audio::linein_stop();
}

static void _linein_resume() {
	audio::linein_start();
}

static void _bt_music_pause() {
	bt::music_pause();
}

static void _bt_music_resume() {
	bt::music_play();
}

static audio_reverse_t _s_audio_reverse_tab[] = {
	{ E_AUDIO_TYPE_BT_MUSIC, false, bt::music_is_playing, _bt_music_pause, _bt_music_resume },
	{ E_AUDIO_TYPE_MUSIC, false, media::music_is_playing, media::music_pause, media::music_resume },
	{ E_AUDIO_TYPE_LYLINK_MUSIC, false, lk::music_is_playing, lk::music_pause, lk::music_resume },
	{ E_AUDIO_TYPE_LINEIN, false, audio::linein_is_playing, _linein_pause, _linein_resume },
};

// 保存当前所有音量状态
static void save_volume_states() {
	vol_record.system_vol = audio::get_system_vol();
	// 根据当前通话状态获取相应音量
	if (bt::is_calling()) {
		// vol_record.bt_call_vol = audio::get_lylink_call_vol();
	}
	if (lk::is_connected() && lk::get_is_call_state() != CallState_Hang) {
		// vol_record.lylink_call_vol = audio::get_lylink_call_vol();
	}
}

static void reverse_show() {
	LOGE("[reverse] link music_is_playing = %d", lk::music_is_playing());
	
	// 保存当前音量状态
	save_volume_states();
	
	for (size_t i = 0; i < TAB_SIZE(_s_audio_reverse_tab); ++i) {
		if (audio::get_audio_type() == _s_audio_reverse_tab[i].type) {
			if (_s_audio_reverse_tab[i].is_playing()) {
				_s_audio_reverse_tab[i].pause();
				LOGE("[reverse] link music_is_playing = %d", lk::music_is_playing());
				_s_audio_reverse_tab[i].playing = true;
				return ;
			}
			_s_audio_reverse_tab[i].playing = false;
		}
	}
}

static void reverse_quit() {
	for (size_t i = 0; i < TAB_SIZE(_s_audio_reverse_tab); ++i) {
		if (audio::get_audio_type() == _s_audio_reverse_tab[i].type) {
			if (!_s_audio_reverse_tab[i].is_playing() && _s_audio_reverse_tab[i].playing) {
				_s_audio_reverse_tab[i].resume();
				return ;
			}
		}
	}
}
/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	//{0,  6000}, //定时器id=0, 时间间隔6秒
};

/**
 * 当界面构造时触发
 */
static void onUI_init() {
    MEM_LIFECYCLE("reverse", "init");
    MEM_VM_DETAIL("reverse_init_ENTRY");
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	fold_statusbar();
//	screenOn_event();
	mCameraViewReversePtr->setErrorCodeCallback(&sMyErrorCodeCallback);
	mCameraViewReversePtr->setPictureCallback(&sMyPictureCallback);
	mCameraViewReversePtr->setDevPath(sys::setting::get_camera_dev());
	mCameraViewReversePtr->setChannel(sys::setting::get_camera_chn());
	mCameraViewReversePtr->setFormatSize(sys::setting::get_camera_wide(),sys::setting::get_camera_high());
	mCameraViewReversePtr->setRotation((ERotation)sys::setting::get_camera_rot());
	mCameraViewReversePtr->setFrameRate(sys::setting::get_camera_rate());
	mCameraViewReversePtr->setOption("req_bufs_count", "3");
	mCameraViewReversePtr->setOption("mem_type", "2");
	char buf[16] = {0};
	sprintf(buf, "%dx%d", sys::setting::get_camera_re_wide(), sys::setting::get_camera_re_high());
	mCameraViewReversePtr->setOption("resize", buf);
	auto format = sys::setting::get_camera_format();
	// 通过环境变量设置开启关闭奇偶合并功能，目前N制P制才使能，切换到其他格式前需要置为0，
	if(format.find("CVBS") != std::string::npos) {
		setenv("ZKCAMERA_DI_ENABLE", "1", 1);
	} else {
		setenv("ZKCAMERA_DI_ENABLE", "0", 1);
	}

//	if(strcmp(sys::setting::get_camera_dev(), "/dev/video4") == 0) { // f133自带的cvbs不需要设置
//		mCameraViewReversePtr->setOption("mem_type", "1");
//	} else {
//		mCameraViewReversePtr->setOption("mem_type", "2");
//	}
//	// 压缩1080p摄像头图层显示为屏幕大小
//	if (sys::setting::get_camera_wide() >= 1920 && sys::setting::get_camera_high() >= 1080) {
//		mCameraViewReversePtr->setOption("resize", sys::hw::getScreenResolution());
//	} else {
//		mCameraViewReversePtr->setOption("resize", "0x0");
//	}

	if (sys::setting::is_reverse_line_view()) {
		mLinePainterPtr->setVisible(true);
		_draw_reverse_line();
	} else {
		mLinePainterPtr->setVisible(false);
	}

	// 初始化音量记录结构
	vol_record.system_vol = audio::get_system_vol();
	// vol_record.bt_call_vol = audio::get_lylink_call_vol();  // 初始化为0，在实际通话时获取
	// vol_record.lylink_call_vol = audio::get_lylink_call_vol();  // 初始化为0，在实际通话时获取

//	effect = (lk::get_lylink_type() == LINK_TYPE_AIRPLAY) || (lk::get_lylink_type() == LINK_TYPE_MIRACAST)
//			|| (lk::get_lylink_type() == LINK_TYPE_WIFILY);
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
	MEM_LIFECYCLE("reverse", "show");
	MEM_WARN_IF_LOW("reverse_show", 3000);
//	if (effect) {
//		audio::set_system_vol(0, effect);
//	}
	bool effect = bt::is_calling() || (lk::is_connected() && lk::get_is_call_state() != CallState_Hang);
	
	// 根据当前状态调整相应音量
	if (bt::is_calling()) {
		// 蓝牙通话中，静音系统音量，保持通话音量
		// audio::set_system_vol(0, !effect);
	} else if (lk::is_connected() && lk::get_is_call_state() != CallState_Hang) {
		// Lylink通话中，静音系统音量，保持Lylink通话音量
		// audio::set_system_vol(0, !effect);
	} else {
		// 非通话状态，静音系统音量
		audio::set_system_vol(0, !effect);
	}
	fy::drop_caches();



//	reverse_show();
	mActivityPtr->registerUserTimer(DELAY_START_PREVIEM_TIMER, 0);
	if (sys::setting::is_reverse_topbar_show()) {
		app::hide_topbar();
	}
	if (bt::is_calling()) {
		if (lk::is_connected()) {
			EASYUICONTEXT->hideStatusBar();
		} else {
			EASYUICONTEXT->showStatusBar();
		}
	}
	EASYUICONTEXT->screensaverOff();
	BRIGHTNESSHELPER->screenOn();
	mActivityPtr->registerUserTimer(FORCE_HIDE_TOPBAR_TIMER, 900);
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	MEM_LIFECYCLE("reverse", "hide");
	WAIT(!mCameraViewReversePtr->isPreviewing(), 100, 50);
}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	MEM_LIFECYCLE("reverse", "quit");
	MEM_WARN_IF_LOW("reverse_quit_BEFORE_RESTORE", 3000);
	mCameraViewReversePtr->setErrorCodeCallback(NULL);
	// 恢复对应的音量设置
	bool effect = bt::is_calling() || (lk::is_connected() && lk::get_is_call_state() != CallState_Hang);
	if (bt::is_calling()) {
		// audio::set_system_vol(vol_record.system_vol, true);
		// bt::call_vol(vol_record.bt_call_vol);
		if(vol_record.system_vol != 0){
			audio::set_system_vol(vol_record.system_vol, true);
		}
	} else if (lk::is_connected() && lk::get_is_call_state() != CallState_Hang) {
		// audio::set_system_vol(vol_record.system_vol, true);
		// audio::set_lylink_call_vol(vol_record.lylink_call_vol, true);
		if(vol_record.system_vol != 0){
			audio::set_system_vol(vol_record.system_vol, true);
		}
	} else {
		// 恢复系统音量
		audio::set_system_vol(vol_record.system_vol, true);
	}
	if (sys::setting::is_reverse_topbar_show()) {
		app::show_topbar();
	}
//	reverse_quit();

	if (bt::is_calling() && lk::get_lylink_type() != LINK_TYPE_WIFIAUTO) {
		EASYUICONTEXT->openActivity("callingActivity");
	}
//  强制清理内存 - 防止OOM导致的黑屏
	fy::drop_caches();
// 	usleep(200000); // 100ms~200ms - 给系统更多时间清理内存

// //  再次强制清理，确保内存充足
// 	fy::drop_caches();
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
	case DELAY_START_PREVIEM_TIMER:
		mCameraViewReversePtr->startPreview();
		return false;
	case FORCE_HIDE_TOPBAR_TIMER:
		if (app::is_show_topbar()) {
			LOGW("[reverse] Force hiding topbar in reverse view");
			app::hide_topbar();
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
static bool onreverseActivityTouchEvent(const MotionEvent &ev) {

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

static int getListItemCount_ListView1(const ZKListView *pListView) {
    //LOGD("getListItemCount_ListView1 !\n");
	 return TABLESIZE(_s_cam_info_tab);
}

static void obtainListItemData_ListView1(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ ListView1  !!!\n");
	pListItem->setText(_s_cam_info_tab[index].name);
	pListItem->setSelected(_s_cam_info_tab[index].checked);

}

static void onListItemClick_ListView1(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ ListView1  !!!\n");

	mCameraViewReversePtr->stopPreview();
	WAIT(!mCameraViewReversePtr->isPreviewing(), 100, 50);

	mCameraViewReversePtr->setFormatSize(_s_cam_info_tab[index].w, _s_cam_info_tab[index].h);
	mCameraViewReversePtr->setFrameRate(_s_cam_info_tab[index].rate);

#if 1
	// 通过环境变量设置开启关闭奇偶合并功能，目前N制P制才使能，切换到其他格式前需要置为0，可对比开启关闭后的效果，权衡下是否打开
	setenv("ZKCAMERA_DI_ENABLE", _s_cam_info_tab[index].di_enable ? "1" : "0", 1);
#endif

	mCameraViewReversePtr->startPreview();

	for (int i = 0; i < (int)TABLESIZE(_s_cam_info_tab); ++i) {
		_s_cam_info_tab[i].checked = (i == index);
	}
}
