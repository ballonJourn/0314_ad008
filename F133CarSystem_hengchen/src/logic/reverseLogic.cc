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
#include "manager/ConfigManager.h"

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

/**
 * 双画布倒车线绘制方案 — 内存优化核心
 *
 * 问题: 1600x600单画布 framebuffer = 1600*600*4 = 3.84MB
 *       F133 ~54MB RAM, 与camera v4l2 buffer争夺 → 进倒车卡顿
 *
 * 方案: 拆为左右两个最小矩形画布, 仅覆盖线条实际绘制区域
 *   mLinePainterPtr  → 左侧倒车线
 *   mLinePainter2Ptr → 右侧倒车线
 *
 * 内存对比:
 *   原方案: 1600 × 600 × 4 = 3,840,000 bytes (3.66MB)
 *   新方案: 2 × (230 × 400 × 4) = 736,000 bytes (0.70MB)
 *   节省: 3.14MB (81.7%)
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │                      1600 × 600 屏幕                         │
 * │                                                              │
 * │  ┌─LEFT─┐                                      ┌─RIGHT─┐   │
 * │  │220,200│                                      │1150,200│   │
 * │  │      │  (中间无倒车线, 不分配framebuffer)    │       │   │
 * │  │230×400│                                      │230×400 │   │
 * │  │      │                                      │       │   │
 * │  └──────┘                                      └───────┘   │
 * └──────────────────────────────────────────────────────────────┘
 *
 * ZK IDE 设置 (两个长方形画布):
 * ┌────────────┬──────┬──────┬───────┬────────┐
 * │ 控件       │  X   │  Y   │ Width │ Height │
 * ├────────────┼──────┼──────┼───────┼────────┤
 * │ LinePainter│  220 │  200 │  230  │  400   │
 * │ LinePainter2│1150 │  200 │  230  │  400   │
 * └────────────┴──────┴──────┴───────┴────────┘
 */

// ============================================================================
// 双画布布局参数 — 必须与 reverse.ftu 中控件的 x, y, w, h 严格一致
// ============================================================================

// 左画布 (覆盖左侧倒车线区域)
// 线条x范围: LBX=240 → LTX+CORNER=430, 加20px余量
// 线条y范围: LTY=220 → LBY=600, 加20px上余量
#define LEFT_CANVAS_X       220    // 画布左上角屏幕x
#define LEFT_CANVAS_Y       200    // 画布左上角屏幕y
#define LEFT_CANVAS_W       230    // 画布宽 (覆盖 220→450, 包裹240~430)
#define LEFT_CANVAS_H       400    // 画布高 (覆盖 200→600, 包裹220~600)

// 右画布 (覆盖右侧倒车线区域, 与左侧镜像对称)
// 线条x范围: RTX-CORNER=1170 → RBX=1360, 加20px余量
#define RIGHT_CANVAS_X     1150    // 画布左上角屏幕x
#define RIGHT_CANVAS_Y      200    // 画布左上角屏幕y
#define RIGHT_CANVAS_W      230    // 画布宽 (覆盖 1150→1380, 包裹1170~1360)
#define RIGHT_CANVAS_H      400    // 画布高 (覆盖 200→600, 包裹220~600)

static void _draw_reverse_line() {
	SZKPoint lt, rt, lb, rb;
	sys::setting::get_reverse_line_point(lt, rt, lb, rb);

	SZKPoint points[3];

	int h = lb.y - lt.y;  // 垂直高度
	if (h <= 0) return;   // 安全守卫: 防止除零和无效坐标

	int gh = REVERSE_LINE_G_RATIO * h;
	int yh = REVERSE_LINE_Y_RATIO * h;

	// ========== 左侧倒车线 → mLinePainterPtr ==========
	// 屏幕坐标 → 画布本地坐标: local_x = screen_x - LEFT_CANVAS_X
	//                          local_y = screen_y - LEFT_CANVAS_Y
	if (mLinePainterPtr) {
		mLinePainterPtr->setLineWidth(REVERSE_LINE_WIDTH);

		int w = lb.x - lt.x;
		float ratio = (float) w / h;

		// 绿色段 (远距离)
		points[0].x = (lt.x + REVERSE_LINE_CORNER_LEN) - LEFT_CANVAS_X;
		points[0].y = lt.y - LEFT_CANVAS_Y;
		points[1].x = lt.x - LEFT_CANVAS_X;
		points[1].y = lt.y - LEFT_CANVAS_Y;
		points[2].x = (int)(ratio * gh + lt.x) - LEFT_CANVAS_X;
		points[2].y = (lt.y + gh) - LEFT_CANVAS_Y;
		mLinePainterPtr->setSourceColor(REVERSE_LINE_G_COLOR);
		mLinePainterPtr->drawLines(points, 3);

		// 黄色段 (中距离)
		points[0].x = points[2].x + REVERSE_LINE_CORNER_LEN;
		points[0].y = points[2].y;
		points[1].x = points[2].x;
		points[1].y = points[2].y;
		points[2].x = (int)(ratio * (gh + yh) + lt.x) - LEFT_CANVAS_X;
		points[2].y = (lt.y + (gh + yh)) - LEFT_CANVAS_Y;
		mLinePainterPtr->setSourceColor(REVERSE_LINE_Y_COLOR);
		mLinePainterPtr->drawLines(points, 3);

		// 红色段 (近距离)
		points[0].x = points[2].x + REVERSE_LINE_CORNER_LEN;
		points[0].y = points[2].y;
		points[1].x = points[2].x;
		points[1].y = points[2].y;
		points[2].x = lb.x - LEFT_CANVAS_X;
		points[2].y = lb.y - LEFT_CANVAS_Y;
		mLinePainterPtr->setSourceColor(REVERSE_LINE_R_COLOR);
		mLinePainterPtr->drawLines(points, 3);
	}

	// ========== 右侧倒车线 → mLinePainter2Ptr ==========
	// 屏幕坐标 → 画布本地坐标: local_x = screen_x - RIGHT_CANVAS_X
	//                          local_y = screen_y - RIGHT_CANVAS_Y
	if (mLinePainter2Ptr) {
		mLinePainter2Ptr->setLineWidth(REVERSE_LINE_WIDTH);

		int w = rb.x - rt.x;
		float ratio = (float) w / h;

		// 绿色段
		points[0].x = (rt.x - REVERSE_LINE_CORNER_LEN) - RIGHT_CANVAS_X;
		points[0].y = rt.y - RIGHT_CANVAS_Y;
		points[1].x = rt.x - RIGHT_CANVAS_X;
		points[1].y = rt.y - RIGHT_CANVAS_Y;
		points[2].x = (int)(ratio * gh + rt.x) - RIGHT_CANVAS_X;
		points[2].y = (rt.y + gh) - RIGHT_CANVAS_Y;
		mLinePainter2Ptr->setSourceColor(REVERSE_LINE_G_COLOR);
		mLinePainter2Ptr->drawLines(points, 3);

		// 黄色段
		points[0].x = points[2].x - REVERSE_LINE_CORNER_LEN;
		points[0].y = points[2].y;
		points[1].x = points[2].x;
		points[1].y = points[2].y;
		points[2].x = (int)(ratio * (gh + yh) + rt.x) - RIGHT_CANVAS_X;
		points[2].y = (rt.y + (gh + yh)) - RIGHT_CANVAS_Y;
		mLinePainter2Ptr->setSourceColor(REVERSE_LINE_Y_COLOR);
		mLinePainter2Ptr->drawLines(points, 3);

		// 红色段
		points[0].x = points[2].x - REVERSE_LINE_CORNER_LEN;
		points[0].y = points[2].y;
		points[1].x = points[2].x;
		points[1].y = points[2].y;
		points[2].x = rb.x - RIGHT_CANVAS_X;
		points[2].y = rb.y - RIGHT_CANVAS_Y;
		mLinePainter2Ptr->setSourceColor(REVERSE_LINE_R_COLOR);
		mLinePainter2Ptr->drawLines(points, 3);
	}
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
	// [FIX] 补偿屏幕旋转，防止 camera overlay 被 DE 和 setRotation 双重旋转
	// 原理: package.properties 中 rotateScreen:270 使 Display Engine 旋转所有图层(含camera overlay)
	//       若 camera 再叠加自身 setRotation(3=270°), 总旋转=270+270=540=180°, 画面倒转
	// 公式: adjusted = (physical_cam_rot - screen_rot + 4) % 4
	//   例: physical=3(270°), screen=3(270°) → adjusted=(3-3+4)%4=0 → 正常
	//        physical=3(270°), screen=0(0°)   → adjusted=(3-0+4)%4=3 → 保持原行为
	{
		int cam_rot = sys::setting::get_camera_rot();            // 物理相机旋转需求 (0-3)
		int screen_rot = CONFIGMANAGER->getScreenRotate() / 90;  // 屏幕旋转 (0-3)
		int adjusted_rot = (cam_rot - screen_rot + 4 + 1) % 4;  // 额外逆时针旋转90°
		mCameraViewReversePtr->setRotation((ERotation)adjusted_rot);
		LOGD("[reverse] cam_rot=%d, screen_rot=%d(x90°), adjusted_rot=%d\n",
			cam_rot, screen_rot, adjusted_rot);
	}
	mCameraViewReversePtr->setFrameRate(sys::setting::get_camera_rate());
	// [FIX] 互联模式下降低camera buffer数量，防止SunxiMemPalloc失败
	// 正常: 3个buffer (720×576×1.5×3 = 1.78MB) — 三缓冲，DMA/显示/备用各一个
	// 互联: 2个buffer (720×576×1.5×2 = 1.18MB) — 双缓冲，省608KB给lylink资源释放
	// CVBS信号25/30fps，双缓冲完全满足实时显示需求，不会产生可感知的撕裂或丢帧
	if (lk::is_connected()) {
		mCameraViewReversePtr->setOption("req_bufs_count", "2");
	} else {
		mCameraViewReversePtr->setOption("req_bufs_count", "3");
	}
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
		if (mLinePainterPtr) mLinePainterPtr->setVisible(true);
		if (mLinePainter2Ptr) mLinePainter2Ptr->setVisible(true);
		_draw_reverse_line();
	} else {
		if (mLinePainterPtr) mLinePainterPtr->setVisible(false);
		if (mLinePainter2Ptr) mLinePainter2Ptr->setVisible(false);
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

	bool effect = bt::is_calling() || (lk::is_connected() && lk::get_is_call_state() != CallState_Hang);
	
	// [FIX] 暂停音乐播放——必须在camera startPreview之前执行
	// 原因: reverse_show() 被注释掉后，音乐播放器在倒车期间持续运行：
	// 1. 音频解码线程持续malloc分配解码缓冲区
	// 2. 音频DMA buffer占用内存
	// 3. 与camera v4l2 buffer争夺仅剩的几MB内存 → OOM → 卡死
	reverse_show();

	// 根据当前状态调整相应音量
	if (bt::is_calling()) {
		// 蓝牙通话中，不静音
	} else if (lk::is_connected() && lk::get_is_call_state() != CallState_Hang) {
		// Lylink通话中，不静音
	} else {
		// 非通话状态，静音系统音量
		audio::set_system_vol(0, !effect);
	}
	fy::drop_caches();

	// [FIX] 延迟startPreview，给lylink音频/视频资源释放留出时间
	// 问题: reverse_show()中调用lk::music_pause()发送LYLINK_AUDIO_NATIVE命令给手机,
	//   手机端响应OnAudioStop并关闭音频流是异步的(约200-500ms)。
	//   旧代码registerUserTimer(DELAY_START_PREVIEM_TIMER, 0)立即启动camera预览，
	//   此时lylink audio player还未执行audio_close释放内存，
	//   加上lylinkview_hide时Aicast的视频流也未停止(已在lylinkviewLogic中修复),
	//   导致FreeMem不足→SunxiMemPalloc fail→camera retry死循环→画面抖动
	// 修复: 给200ms延迟，确保音频资源释放完成后再启动camera
	mActivityPtr->registerUserTimer(DELAY_START_PREVIEM_TIMER, 200);
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
		if(vol_record.system_vol != 0){
			audio::set_system_vol(vol_record.system_vol, true);
		}
	} else if (lk::is_connected() && lk::get_is_call_state() != CallState_Hang) {
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

	// [FIX] 恢复被暂停的音乐/音频播放
	// 与 onUI_show 中的 reverse_show() 配对
	reverse_quit();

	if (bt::is_calling() && lk::get_lylink_type() != LINK_TYPE_WIFIAUTO) {
		EASYUICONTEXT->openActivity("callingActivity");
	}
//  强制清理内存 - 防止OOM导致的黑屏
	fy::drop_caches();
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
