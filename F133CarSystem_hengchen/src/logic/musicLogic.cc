#pragma once
#include "uart/ProtocolSender.h"
/*
*此文件由GUI工具生成
*文件功能：用于处理用户的逻辑相应代码
*功能说明：
*========================onButtonClick_XXXX
当页面中的按键按下后系统会调用对应的函数，XXX代表GUI工具里面的[ID值]名称，
如Button1,当返回值为false的时候系统将不再处理这个按键，返回true的时候系统将会继续处理此按键。比如SYS_BACK.
*========================onSlideWindowItemClick_XXXX(int index) 
当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如slideWindow1;index 代表按下图标的偏移值
*========================onSeekBarChange_XXXX(int progress) 
当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如SeekBar1;progress 代表当前的进度值
*========================ogetListItemCount_XXXX() 
当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称，
如List1;返回值为当前列表的总条数
*========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称，
如List1;pListItem 是贴图中的单条目对象，index是列表总目的偏移量。具体见函数说明
*========================常用接口===============
*LOGD(...)  打印调试信息的接口
*mTextXXXPtr->setText("****") 在控件TextXXX上显示文字****
*mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式，图片会切换成选中图片，按钮文字会切换为选中后的颜色
*mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
*mListView1Ptr->refreshListView() 让mListView1 重新刷新，当列表数据变化后调用
*mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
*
* 在Eclipse编辑器中  使用 "alt + /"  快捷键可以打开智能提示
*/

#include "link/context.h"
#include "media/audio_context.h"
#include "media/media_context.h"
#include "media/media_parser.h"
#include "media/music_player.h"
#include "fy/files.hpp"
#include "fy/os.hpp"
#include "media/media_parser.h"
#include "manager/ConfigManager.h"
#include "utils/Loading_icon.hpp"
#include "utils/BitmapHelper.h"
#include "uart/context.h"
#include "system/setting.h"
#include "mode_observer.h"
#include "bt/context.h"
#include "utils/mem_profiler.h"

static storage_type_e _s_select_storage = E_STORAGE_TYPE_SD; //E_STORAGE_TYPE_INVALID;	//选中的文件仓库
static storage_type_e _s_play_storage = E_STORAGE_TYPE_SD; //E_STORAGE_TYPE_INVALID;		//播放的文件仓库

static const char* play_mode_path[] = {"media_player/cycle_n.png", "media_player/single_n.png", "media_player/random_n.png"};
static const char* play_mode_press_path[] = {"media_player/cycle_p.png", "media_player/single_p.png", "media_player/random_p.png"};

static bool is_tracking = false;
static int track_progress = -1;
static bool is_back = false;

// 新增：记录进入音乐播放界面的方式
typedef enum {
    E_ENTER_FROM_APP = 0,      // 从主应用界面进入
    E_ENTER_FROM_LIST = 1      // 从音乐列表进入
} music_enter_type_e;

static music_enter_type_e enter_type = E_ENTER_FROM_APP;
static music_enter_type_e original_enter_type = E_ENTER_FROM_APP;  // 新增：保存原始进入方式

namespace {
class SeekbarChangeListener: public ZKSeekBar::ISeekBarChangeListener {
public:
	virtual void onProgressChanged(ZKSeekBar *pSeekBar, int progress) {
		track_progress = progress;
		if (media::music_get_play_index() != -1) {
	        char timeStr[10];
//	        int pos = pSeekBar->getProgress();
	        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", progress / 60, progress % 60);
	        mCurPosTextViewPtr->setText(timeStr);
		}
	}
	virtual void onStartTrackingTouch(ZKSeekBar *pSeekBar) {
		is_tracking = true;
	}
	virtual void onStopTrackingTouch(ZKSeekBar *pSeekBar) {
		is_tracking = false;
		if (track_progress >= 0) {
			media::music_seek(track_progress * 1000);
			track_progress = -1;
		}
	}
};

}
static SeekbarChangeListener progressbar;

static void _register_timer_fun(int id, int time) {
	mActivityPtr->registerUserTimer(id, time); // @suppress("无效参数")
}

//旋转加载图标
static zkf::IconRotate iconRotate;

// 解析id3显示
static void refreshMusicInfo() {
	std::string cur_play_file = media::music_get_current_play_file();

	id3_info_t info;
	memset(&info, 0, sizeof(id3_info_t));
	bool isTrue = media::parse_id3_info(cur_play_file.c_str(), &info);
	if (isTrue && strcmp(info.title, "") != 0) {
		mtitleTextViewPtr->setText(info.title);
	} else {
		mtitleTextViewPtr->setText(fy::files::get_file_name(cur_play_file));
	}

	if (isTrue) {
		(strcmp(info.artist, "") == 0) ? martistTextViewPtr->setTextTr("Unknown") : martistTextViewPtr->setText(info.artist);
		(strcmp(info.album, "") == 0) ? malbumTextViewPtr->setTextTr("Unknown") : malbumTextViewPtr->setText(info.album);
	}
	else {
		martistTextViewPtr->setTextTr("Unknown");
		malbumTextViewPtr->setTextTr("Unknown");
	}

	isTrue = media::parse_id3_pic(cur_play_file.c_str(), "/tmp/m1.jpg");
	MEM_IMG_LOAD_BEGIN(albumart);
	mpicTextViewPtr->setBackgroundPic(NULL);
	mpicTextViewPtr->setBackgroundPic(isTrue ? "/tmp/m1.jpg" : CONFIGMANAGER->getResFilePath("media_player/icon_media_cover_n.png").c_str());
	MEM_IMG_LOAD_END("music_albumart", albumart, isTrue ? "/tmp/m1.jpg" : "icon_media_cover_n.png");
	mmusicListViewPtr->refreshListView();
}

static void setDuration() {
    int max = media::music_get_duration() / 1000;
    char timeStr[12] = {0};
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", max / 60, max % 60);
    mDurationTextViewPtr->setText(timeStr);
    mPlayProgressSeekbarPtr->setMax(max);
}

//按键单选
static void muont_path_button_selected(storage_type_e _s_select_type) {
	ZKButton** pBtnArr[] = {&mSDButtonPtr, &mUSB2ButtonPtr, &mUSB1ButtonPtr};
	for (size_t i=0; i<TABLESIZE(pBtnArr); i++) {
		(*pBtnArr[i])->setSelected(i == (int)_s_select_type);
	}
}

//刷新界面,跳转至当前播放
static void seek_to_current_play() {
    _s_select_storage = _s_play_storage;
    muont_path_button_selected(_s_play_storage);
    int play_index = media::music_get_play_index();
    mmusicListViewPtr->setSelection((play_index > (int)mmusicListViewPtr->getRows()-1) ? play_index-3 : 0);  //小于0时,跳转到0
    mmusicListViewPtr->refreshListView();
}

static void _show_list_area_controls() {
	if (mmusicListViewPtr) mmusicListViewPtr->setVisible(true);
	// mTextView7Ptr 可能已从FTU删除(背景转移到ListView), 兼容处理
	if (mTextView7Ptr) mTextView7Ptr->setVisible(true);
	if (mSDButtonPtr) mSDButtonPtr->setVisible(true);
	if (mUSB1ButtonPtr) mUSB1ButtonPtr->setVisible(true);
	// mUSB2ButtonPtr 默认 hidden (FTU中 visible=false), 不恢复
	if (mTextView1Ptr) mTextView1Ptr->setVisible(true);
	if (mTextView2Ptr) mTextView2Ptr->setVisible(true);
	if (mTextView3Ptr) mTextView3Ptr->setVisible(true);
	// mTextView4Ptr 默认 hidden (FTU中 visible=false), 不恢复
}

// ============================================================================
// 列表区域可见性管理
//
// 背景: musicWindow 从 1600x600 缩小到实际播放界面所需尺寸 (约 602x470)
// 以节省 ~2.5MB framebuffer. 缩小后 musicWindow 不再覆盖整个屏幕,
// 列表区域的控件会从 musicWindow 边缘"露出来".
//
// 解决: musicWindow 显示时隐藏列表区控件, 隐藏时恢复.
// 注意: setVisible 不释放framebuffer, 只跳过绘制和合成, 但能防止视觉穿透
//       并减轻 compositor 合成压力 (少合一层 = 少一次 blit)
// ============================================================================
static void _hide_list_area_controls() {
	if (mmusicListViewPtr) mmusicListViewPtr->setVisible(false);
	if (mTextView7Ptr) mTextView7Ptr->setVisible(false);
	if (mSDButtonPtr) mSDButtonPtr->setVisible(false);
	if (mUSB1ButtonPtr) mUSB1ButtonPtr->setVisible(false);
	if (mUSB2ButtonPtr) mUSB2ButtonPtr->setVisible(false);
	if (mTextView1Ptr) mTextView1Ptr->setVisible(false);  // tab_line 分隔线
	if (mTextView2Ptr) mTextView2Ptr->setVisible(false);  // icon_sd
	if (mTextView3Ptr) mTextView3Ptr->setVisible(false);  // icon_usb
	if (mTextView4Ptr) mTextView4Ptr->setVisible(false);  // icon_input
}

static void _music_play_status_cb(music_play_status_e status) {
	switch (status) {
	case E_MUSIC_PLAY_STATUS_STARTED:    // 播放开始
		setDuration();
		mPlayProgressSeekbarPtr->setProgress(0);
		refreshMusicInfo();
		mButtonPlayPtr->setSelected(true);
		mtitleTextViewPtr->setTextColor(0xFF00FCFF);
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_MUSIC);
		break;

	case E_MUSIC_PLAY_STATUS_RESUME:     // 恢复播放
		mButtonPlayPtr->setSelected(true);
		mtitleTextViewPtr->setTextColor(0xFF00FCFF);
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_MUSIC);
		break;

	case E_MUSIC_PLAY_STATUS_PAUSE:      // 暂停播放
		mButtonPlayPtr->setSelected(false);
		mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		break;

	case E_MUSIC_PLAY_STATUS_STOP:       // 停止播放
		setDuration();
		refreshMusicInfo();
		mButtonPlayPtr->setSelected(false);
		mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		mmusicWindowPtr->hideWnd();
		_show_list_area_controls();
		break;

	case E_MUSIC_PLAY_STATUS_ERROR:{// 播放错误
		merrorWindowPtr->showWnd();
// 		启动定时器自动隐藏错误窗口,播放错误时自动播放下一曲
		mActivityPtr->registerUserTimer(2, 2000); // 2秒后隐藏
		media::music_next(true);
	}
		break;

	case E_MUSIC_PLAY_STATUS_COMPLETED:  // 播放结束
		media::music_next();
		break;
	}
}

static void _media_scan_cb(const char *dir, storage_type_e type, bool started) {
	if (started) {
		iconRotate.run();
	} else {
//		iconRotate.requestExit();
		iconRotate.requestExitAndWait();
	}
	mscaningWindowPtr->setVisible(started);
	mmusicListViewPtr->refreshListView();
}

//控件初始化
static void ctrl_init() {
	merrorTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	martistTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	malbumTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView2Ptr->setTouchPass(true);
	mTextView3Ptr->setTouchPass(true);
	mTextView4Ptr->setTouchPass(true);
}

static void _event_mode_cb(event_mode_e mode) {
	if (E_EVENT_MODE_GOBACK == mode) {
		if (mmusicWindowPtr->isWndShow()) {
			// 如果音乐窗口正在显示，根据进入方式决定返回行为
			if (enter_type == E_ENTER_FROM_LIST) {
				// 从列表进入的，返回列表
				mmusicWindowPtr->hideWnd();
				_show_list_area_controls();
				enter_type = E_ENTER_FROM_APP;  // 重置状态
				mode::set_switch_mode(E_SWITCH_MODE_GOBACK);  // 回到列表后设置为goback模式
			} else {
				// 从主应用进入的，直接返回主应用
				mmusicWindowPtr->hideWnd();
				_show_list_area_controls();
				EASYUICONTEXT->goBack();
			}
		} else {
			// 音乐窗口没有显示，当前在列表界面
			if (is_back) {
				_hide_list_area_controls();
				mmusicWindowPtr->showWnd();
				// 修复：恢复原始进入方式，而不是强制设置为E_ENTER_FROM_LIST
				enter_type = original_enter_type;
				mode::set_switch_mode(E_SWITCH_MODE_GOBACK);
				is_back = false;
			} else {
				EASYUICONTEXT->goBack();
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
	{1,  1000},
	{2,  2000}, // 新增：错误窗口自动隐藏定时器，2秒后隐藏
};

/**
 * 当界面构造时触发
 */
static void onUI_init() {
    MEM_LIFECYCLE("music", "init");
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	ctrl_init();
	iconRotate.SetCtrl(msyncPointerPtr, mscaningWindowPtr);

	mplayModeButtonPtr->setButtonStatusPic(ZK_CONTROL_STATUS_NORMAL, play_mode_path[media::music_get_play_mode()]);
//	mplayModeButtonPtr->setButtonStatusPic(ZK_CONTROL_STATUS_PRESSED, play_mode_press_path[media::music_get_play_mode()]);

	media::music_add_play_status_cb(_music_play_status_cb);
	media::add_scan_cb(_media_scan_cb);
	mode::add_event_mode_cb(_event_mode_cb);
	mPlayProgressSeekbarPtr->setSeekBarChangeListener(&progressbar);
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
	MEM_LIFECYCLE("music", "show");
	MEM_WARN_IF_LOW("music_show", 4000);
	is_back = false;

	// [FIX] 恢复 onUI_hide 中释放的装饰性图片资源
	// 这些是 FTU 中静态设置的背景图, hide时为腾内存主动释放, show时需恢复
	if (mTextViewPtr) {
		mTextViewPtr->setBackgroundPic("media_player/icon_media_cover_bg_n.png");
	}
	if (mTextView8Ptr) {
		mTextView8Ptr->setBackgroundPic("media_player/progress_n.png");
	}

	// [FIX] onUI_hide中为倒车腾内存释放了专辑封面(setBackgroundPic(NULL)),
	// 退出倒车回来时必须恢复,无论当前是否正在播放——
	// 因为倒车期间 reverse_show() 会暂停音乐, 回来时 music_is_playing()=false,
	// 如果不在这里恢复, 专辑封面将永远空白
	if (media::music_get_play_index() != -1) {
		refreshMusicInfo();
		setDuration();
	}

	if (media::music_is_playing() || media::music_get_play_index() != -1) {
	    mtitleTextViewPtr->setTextColor(0xFF00FCFF);
	    mButtonPlayPtr->setSelected(true);
		_hide_list_area_controls();
		mmusicWindowPtr->showWnd();

		// 如果音乐正在播放或有播放记录，说明是从主应用界面进入的
		enter_type = E_ENTER_FROM_APP;
		original_enter_type = E_ENTER_FROM_APP;  // 同时设置原始进入方式
		mode::set_switch_mode(E_SWITCH_MODE_NULL);  // 直接返回主应用界面
	} else {
		_show_list_area_controls();
		seek_to_current_play();
		// 此时还在列表界面，没有进入音乐播放窗口
		enter_type = E_ENTER_FROM_APP;  // 默认为从主应用进入
		original_enter_type = E_ENTER_FROM_APP;
		mode::set_switch_mode(E_SWITCH_MODE_GOBACK);
	}
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	MEM_LIFECYCLE("music", "hide");
	MEM_VM_DETAIL("music_hide_ENTRY");

	// [FIX] 释放音乐界面的动态图片资源，为倒车/互联腾出内存
	// 控件 framebuffer 无法释放（直到 Activity quit），
	// 但动态加载的 bitmap 可以释放:

	// 1. 释放专辑封面图片 (来自/tmp/m1.jpg的解码bitmap)
	if (mpicTextViewPtr) {
		mpicTextViewPtr->setBackgroundPic(NULL);
	}

	// 2. 释放 musicWindow 内的装饰性图片资源
	//    这些图片在 onUI_show / refreshMusicInfo 时会重新加载
	if (mTextViewPtr) {
		mTextViewPtr->setBackgroundPic(NULL);   // icon_media_cover_bg_n.png (242x242)
	}
	if (mTextView8Ptr) {
		mTextView8Ptr->setBackgroundPic(NULL);  // progress_n.png 进度条背景
	}

	// 3. 强制释放pagecache，立即为camera腾出空间
	fy::drop_caches();
}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	MEM_LIFECYCLE("music", "quit");
	MEM_VM_DETAIL("music_quit_ENTRY");
	if (iconRotate.isRunning())
		iconRotate.requestExitAndWait();
	mode::remove_event_mode_cb(_event_mode_cb);
	media::remove_scan_cb(_media_scan_cb);
	media::music_remove_play_status_cb(_music_play_status_cb);
	mPlayProgressSeekbarPtr->setSeekBarChangeListener(NULL);
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
    case 1:{
        int curPos = -1;
        if (media::music_is_playing()) {
            curPos = media::music_get_current_position() / 1000;
        }
        else if (media::music_get_play_index() == -1) {
            curPos = 0;
            mDurationTextViewPtr->setText("00:00");
            mButtonPlayPtr->setSelected(false);
            mtitleTextViewPtr->setTextColor(0xFFFFFF);
        }
        if (curPos >= 0) {
//            char timeStr[10];
//            snprintf(timeStr, sizeof(timeStr), "%02d:%02d", curPos / 60, curPos % 60);
//            mCurPosTextViewPtr->setText(timeStr);
            mPlayProgressSeekbarPtr->setProgress(curPos);
        }
    }
        break;
    case 2:{ // 错误窗口自动隐藏
        if (merrorWindowPtr->isWndShow()) {
            merrorWindowPtr->hideWnd();
        }
        return false; // 停止定时器，因为只需要执行一次
    }
        break;
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
static bool onmusicActivityTouchEvent(const MotionEvent &ev) {
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
static bool onButtonClick_SDButton(ZKButton *pButton) {
    LOGD(" ButtonClick SDButton !!!\n");
    _s_select_storage = E_STORAGE_TYPE_SD;
    muont_path_button_selected(_s_select_storage);
    (_s_select_storage == _s_play_storage) ? seek_to_current_play() : mmusicListViewPtr->setSelection(0);
    mmusicListViewPtr->refreshListView();
    return false;
}

static bool onButtonClick_USB1Button(ZKButton *pButton) {
    LOGD(" ButtonClick USB1Button !!!\n");
    _s_select_storage = E_STORAGE_TYPE_USB2;
    muont_path_button_selected(_s_select_storage);
    (_s_select_storage == _s_play_storage) ? seek_to_current_play() : mmusicListViewPtr->setSelection(0);
    mmusicListViewPtr->refreshListView();
    return false;
}

static bool onButtonClick_USB2Button(ZKButton *pButton) {
    LOGD(" ButtonClick USB2Button !!!\n");
    _s_select_storage = E_STORAGE_TYPE_USB1;
    muont_path_button_selected(_s_select_storage);
    (_s_select_storage == _s_play_storage) ? seek_to_current_play() : mmusicListViewPtr->setSelection(0);
    mmusicListViewPtr->refreshListView();
    return false;
}

static int getListItemCount_musicListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_musicListView !\n");
	if (mTextView7Ptr) {
		if (media::get_audio_list_size(_s_select_storage) == 0) {
			mTextView7Ptr->setTextTr("No files");
		} else {
			mTextView7Ptr->setText("");
		}
	}
    return media::get_audio_list_size(_s_select_storage);
}

static void obtainListItemData_musicListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ musicListView  !!!\n");
	pListItem->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	ZKListView::ZKListSubItem *SubItemID = pListItem->findSubItemByID(ID_MUSIC_SubItemID);
	SubItemID->setText(index+1);
	std::string file = media::get_audio_file(_s_select_storage, index);
	pListItem->setText(file.substr(file.rfind("/")+1));
	pListItem->setSelected((_s_select_storage == _s_play_storage) ? (index == media::music_get_play_index()) : false);
	SubItemID->setSelected(pListItem->isSelected());
}

static void onListItemClick_musicListView(ZKListView *pListView, int index, int id) {
    LOGD(" onListItemClick_ musicListView  !!!\n");
//    uart::set_sound_channel(SOUND_CHANNEL_ARM);
	if (bt::is_calling()) {
		return ;
	}
    is_back = false;

    // 保存当前的进入方式作为原始进入方式
    original_enter_type = enter_type;

    // 标记为从列表进入音乐播放界面
    enter_type = E_ENTER_FROM_LIST;

    _hide_list_area_controls();
    mmusicWindowPtr->showWnd();

    // 设置为返回列表模式
    mode::set_switch_mode(E_SWITCH_MODE_GOBACK);

	if (media::music_get_play_index() == index && _s_select_storage == _s_play_storage) {
	    refreshMusicInfo();
	    setDuration();
	    media::music_resume();
	}
	else {
		_s_play_storage = _s_select_storage;
		media::music_play(_s_play_storage, index);
	}
}


static bool onButtonClick_musicCloseButton(ZKButton *pButton) {
    LOGD(" ButtonClick musicCloseButton !!!\n");
    mmusicWindowPtr->hideWnd();
    _show_list_area_controls();
    return false;
}

static void onProgressChanged_PlayProgressSeekbar(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged PlayProgressSeekbar %d !!!\n", progress);
}

static bool onButtonClick_musicListButton(ZKButton *pButton) {
    LOGD(" ButtonClick musicListButton !!!\n");
    // 添加：如果正在蓝牙通话，禁止打开列表
    if (bt::is_calling()) {
        return false;
    }

    is_back = true;

    // 保存当前的进入方式作为原始进入方式
    original_enter_type = enter_type;

    seek_to_current_play();
    mmusicListViewPtr->refreshListView();
    mmusicWindowPtr->hideWnd();
    _show_list_area_controls();

//    回到列表后，重置为主应用进入模式，因为下次进入就是从列表进入了
//    enter_type = E_ENTER_FROM_APP;  // 重置状态
    mode::set_switch_mode(E_SWITCH_MODE_GOBACK);  // 在列表界面时设置为goback模式

    return false;
}

static bool onButtonClick_playModeButton(ZKButton *pButton) {
    LOGD(" ButtonClick playModeButton !!!\n");
    media::music_set_play_mode((media_play_mode_e)((media::music_get_play_mode() + 1) % 3));
    pButton->setButtonStatusPic(ZK_CONTROL_STATUS_NORMAL, play_mode_path[media::music_get_play_mode()]);
    pButton->setButtonStatusPic(ZK_CONTROL_STATUS_PRESSED, play_mode_press_path[media::music_get_play_mode()]);
	return false;
}

static bool onButtonClick_PrevButton(ZKButton *pButton) {
    LOGD(" ButtonClick PrevButton !!!\n");
	if (bt::is_calling()) {
		return false;
	}
    media::music_prev(true);
    return false;
}

static bool onButtonClick_ButtonPlay(ZKButton *pButton) {
    LOGD(" ButtonClick ButtonPlay !!!\n");
	if (bt::is_calling()) {
		return false;
	}
    if (media::music_get_play_index() == -1) {
    	return false;
    }
    else if (media::music_is_playing()) {
        media::music_pause();
    } else {
    	media::music_resume();
    }
    return false;
}

static bool onButtonClick_NextButton(ZKButton *pButton) {
    LOGD(" ButtonClick NextButton !!!\n");
	if (bt::is_calling()) {
		return false;
	}
    media::music_next(true);
    return false;
}
static bool onButtonClick_Button1(ZKButton *pButton) {
    LOGD(" ButtonClick Button1 !!!\n");
    return false;
}


static bool onButtonClick_musicButton(ZKButton *pButton) {
    LOGD(" ButtonClick musicButton !!!\n");
    // 添加：如果正在蓝牙通话，禁止打开音乐播放窗口
    if (bt::is_calling()) {
        return false;
    }

    _hide_list_area_controls();
    mmusicWindowPtr->showWnd();
    mButtonPlayPtr->setSelected(media::music_is_playing());
    media::music_is_playing() ?  mtitleTextViewPtr->setTextColor(0xFF00FCFF) :  mtitleTextViewPtr->setTextColor(0xFFFFFF);
    if (media::music_get_play_index() != -1) {
    	refreshMusicInfo();
    	setDuration();
    	mPlayProgressSeekbarPtr->setProgress(media::music_get_current_position() / 1000);
    }
    return false;
}
