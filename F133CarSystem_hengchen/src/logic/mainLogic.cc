// ad008_mainLogic.cc - Refactored version
// Removed appSlideWindow, added dock window buttons with swipe-based page switching

#pragma once
#include "uart/ProtocolSender.h"
/*
*此文件由GUI工具生成
*文件功能:用于处理用户的逻辑相应代码
*功能说明:
*========================onButtonClick_XXXX
当页面中的按键按下后系统会调用对应的函数,XXX代表GUI工具里面的[ID值]名称,
如Button1,当返回值为false的时候系统将不再处理这个按键,返回true的时候系统将会继续处理此按键。比如SYS_BACK.
*========================onSlideWindowItemClick_XXXX(int index)
当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称,
如slideWindow1;index 代表按下图标的偏移值
*========================onSeekBarChange_XXXX(int progress)
当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称,
如SeekBar1;progress 代表当前的进度值
*========================ogetListItemCount_XXXX()
当页面中存在滑动列表的时候,更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称,
如List1;返回值为当前列表的总条数
*========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候,更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称,
如List1;pListItem 是贴图中的单条目对象,index是列表总目的偏移值。具体见函数说明
*========================常用接口===============
*LOGD(...)  打印调试信息的接口
*mText1Ptr->setText("****") 在控件TextXXX上显示文字****
*mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式,图片会切换成选中图片,按钮文字会切换为选中后的颜色
*mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
*mListView1Ptr->refreshListView() 让mListView1 重新刷新,当列表数据变化后调用
*mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
*
* 在Eclipse编辑器中  使用 "alt + /"  快捷键可以打开智能提示
*/

#include "net/context.h"
#include "link/context.h"
#include "uart/context.h"
#include "bt/context.h"
#include "media/audio_context.h"
#include "media/media_context.h"
#include "media/music_player.h"
#include "media/media_parser.h"
#include "system/setting.h"
#include "system/fm_emit.h"
#include "system/reverse.h"
#include "manager/LanguageManager.h"
#include "manager/ConfigManager.h"
#include "storage/StoragePreferences.h"
#include "misc/storage.h"
#include "fy/files.hpp"
#include "net/NetManager.h"
#include "os/MountMonitor.h"
#include "system/usb_monitor.h"
#include "tire/tire_parse.h"
#include "sysapp_context.h"
#include "utils/BitmapHelper.h"
#include <base/ui_handler.h>
#include "system/hardware.h"
#include <base/mount_notification.h>
#include <base/time.hpp>
#include "utils/TimeHelper.h"
#include "mode_observer.h"
#include "mcu_hash_checker.h"
#include "utils/mem_profiler.h"
#include "utils/SlideManager.h"

#define WIFIMANAGER			NETMANAGER->getWifiManager()

// extern函数声明 - 同步其他界面的SeekBar
extern void set_navibar_brightnessBar(int progress);
extern void set_navibar_PlayVolSeekBar(int progress);
extern void set_ctrlbar_lightSeekBar(int progress);
extern void set_ctrlbar_volumSeekBar(int progress);
extern void setSettingFtu_BrillianceSeekBar(int progress);
extern void setSettingFtu_MediaSeekBar(int progress);

#define KEY_LONG_PRESS_TIMEOUT    3000
#define TIMER_POWERKEY_EVENT 	303
#define TIMER_POWERKEY_OFF		304

#define QUERY_LINK_AUTH_TIMER	3
#define SWITCH_ADB_TIMER	4
#define BT_MUSIC_ID3		5
#define MCU_AUTO_UPGRADE	6
#define MUSIC_ERROR_TIMER	20

#define TIME_SYNCED_FLAG "/data/.time_synced_flag"

#define SCREEN_WIDTH	1600  // 根据实际屏幕宽度调整 (用于滑动切换窗口)

extern void fold_statusbar();

static bt_cb_t _s_bt_cb;
static bool _s_need_reopen_linkview;
static int key_sec = 0;

// **内存优化:添加状态管理变量**
static bool _is_in_reverse_mode = false;      // 倒车状态
static bool _is_ui_update_paused = false;     // UI更新暂停状态
static bool _is_music_info_cached = false;    // 音乐信息缓存状态
static std::string _cached_title = "";        // 缓存的音乐标题
static std::string _cached_artist = "";       // 缓存的艺术家信息
static std::string _last_play_file = "";      // 上次播放的文件
static bool _background_resources_loaded = false; // 背景资源加载状态
static bool _is_exiting_reverse = false;      // 标记是否正在从倒车模式退出

// ============================================================================
// 页面状态管理
// 使用手指滑动距离判断翻页，不再依赖appSlideWindow控件
// dock1Window, dock2Window, dock3Window 分别对应三个页面
// ============================================================================
static int _current_page_index = 0;  // 当前页面索引 (0, 1, 2)

// ============================================================================
// 前向声明 - 按页设置dock按钮背景
// ============================================================================
static void set_dock_button_backgrounds_for_page(int page);

static void switch_dock_window(int page) {
    if (page < 0) page = 0;
    if (page > 2) page = 2;

    _current_page_index = page;
    LOGD("[main] switch_dock_window: page = %d", page);

    // 更新RadioGroup状态
    switch (page) {
    case 0:
        if (mStatusRadioGroupPtr) mStatusRadioGroupPtr->setCheckedID(ID_MAIN_RadioButton0);
        break;
    case 1:
        if (mStatusRadioGroupPtr) mStatusRadioGroupPtr->setCheckedID(ID_MAIN_RadioButton1);
        break;
    case 2:
        if (mStatusRadioGroupPtr) mStatusRadioGroupPtr->setCheckedID(ID_MAIN_RadioButton2);
        break;
    }

    // 显示/隐藏 dock 窗口
    if (mdock1WindowPtr) {
        if (page == 0) mdock1WindowPtr->showWnd();
        else mdock1WindowPtr->hideWnd();
    }
    if (mdock2WindowPtr) {
        if (page == 1) mdock2WindowPtr->showWnd();
        else mdock2WindowPtr->hideWnd();
    }
    if (mdock3WindowPtr) {
        if (page == 2) mdock3WindowPtr->showWnd();
        else mdock3WindowPtr->hideWnd();
    }

    // 只加载当前页的按钮背景，释放其他页的，防止图片过多卡顿
    set_dock_button_backgrounds_for_page(page);
}

// ============================================================================
// 供外部(topbar)调用: 若不在第0页则切回第0页, 返回true表示执行了切换
// ============================================================================
bool handle_main_page_back() {
    if (_current_page_index != 0) {
        switch_dock_window(0);
        return true;
    }
    return false;
}

// ============================================================================
// 切换dock窗口的显示
// page: 0 - dock1Window, 1 - dock2Window, 2 - dock3Window
// ============================================================================


// 根据滑动方向切换页面
static void switch_page_by_direction(int delta_x) {
    int new_page = _current_page_index;

    if (delta_x < 0) {
        // 向左滑动，切换到下一页
        new_page = _current_page_index + 1;
    } else if (delta_x > 0) {
        // 向右滑动，切换到上一页
        new_page = _current_page_index - 1;
    }

    // 边界检查
    if (new_page < 0) new_page = 0;
    if (new_page > 2) new_page = 2;

    if (new_page != _current_page_index) {
        switch_dock_window(new_page);
    }
}

// ============================================================================
// 更新所有连接类按钮的状态文本 (Connected / Not connected)
// ============================================================================
static void update_link_status_text() {
    LYLINK_TYPE_E link_type = lk::get_lylink_type();

    // 更新CarPlay状态 - dock1Window下的CarPlayButton
    if (mCarPlayButtonPtr) {
        if ((link_type == LINK_TYPE_WIFICP) || (link_type == LINK_TYPE_USBCP)) {
            mCarPlayButtonPtr->setText("Connected");
        } else {
            mCarPlayButtonPtr->setText("Not connected");
        }
    }

    // 更新AndroidAuto状态 - dock1Window下的AndroidAutoButton
    if (mAndroidAutoButtonPtr) {
        if ((link_type == LINK_TYPE_WIFIAUTO) || (link_type == LINK_TYPE_USBAUTO)) {
            mAndroidAutoButtonPtr->setText("Connected");
        } else {
            mAndroidAutoButtonPtr->setText("Not connected");
        }
    }

    // 更新Bluetooth状态 - dock1Window下的BluetoothButton
    if (mBluetoothButtonPtr) {
        if (bt::get_connect_state() == E_BT_CONNECT_STATE_CONNECTED) {
            mBluetoothButtonPtr->setText("Connected");
        } else {
            mBluetoothButtonPtr->setText("Not connected");
        }
    }

    // 更新AirPlay状态 - dock2Window下的AirPlayButton
    if (mAirPlayButtonPtr) {
        if (link_type == LINK_TYPE_AIRPLAY) {
            mAirPlayButtonPtr->setText("Connected");
        } else {
            mAirPlayButtonPtr->setText("Not connected");
        }
    }

    // 更新Miracast状态 - dock2Window下的MiracastButton
    if (mMiracastButtonPtr) {
        if (link_type == LINK_TYPE_MIRACAST) {
            mMiracastButtonPtr->setText("Connected");
        } else {
            mMiracastButtonPtr->setText("Not connected");
        }
    }

    // 更新AiCast状态 - dock2Window下的AicastButton
    if (mAicastButtonPtr) {
        if (link_type == LINK_TYPE_WIFILY) {
            mAicastButtonPtr->setText("Connected");
        } else {
            mAicastButtonPtr->setText("Not connected");
        }
    }
}

// ============================================================================
// dock1Window按钮的背景图片设置
// dock1现在包含: musicWindow, musictext, AudiooutButton, CarPlayButton,
//               AndroidAutoButton, BluetoothButton
// ============================================================================
static void set_dock1_button_backgrounds() {
    MEM_SNAP_SIMPLE("dock1_bg_LOAD_START");
    if (mmusicWindowPtr) {
        mmusicWindowPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_music_n.png").c_str());
    }
    if (mAudiooutButtonPtr) {
        mAudiooutButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_audio_out_n.png").c_str());
    }
    if (mCarPlayButtonPtr) {
        mCarPlayButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_carplay_n.png").c_str());
    }
    if (mAndroidAutoButtonPtr) {
        mAndroidAutoButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_android_auto_n.png").c_str());
    }
    if (mBluetoothButtonPtr) {
        mBluetoothButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_bt_n.png").c_str());
    }
}

// ============================================================================
// dock2Window按钮的背景图片设置
// ============================================================================
static void set_dock2_button_backgrounds() {
    if (mAicastButtonPtr) {
        mAicastButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_aicast_n.png").c_str());
    }
    if (mMusicButtonPtr) {
        mMusicButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_music2_n.png").c_str());
    }
    if (mSettingsButtonPtr) {
        mSettingsButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_set_n.png").c_str());
    }
    if (mAirPlayButtonPtr) {
        mAirPlayButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_airplay_n.png").c_str());
    }
    if (mMiracastButtonPtr) {
        mMiracastButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_miracast_n.png").c_str());
    }
}

// ============================================================================
// dock3Window按钮的背景图片设置
// 第三页有 PictureButton 和 videoButton，其余3个位置用占位背景图填充
// (TextViewRightarea2已删除，因为video卡片占据了该位置)
// ============================================================================
static void set_dock3_button_backgrounds() {
    MEM_SNAP_SIMPLE("dock3_bg_LOAD_START");
    if (mPictureButtonPtr) {
        mPictureButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_picture_n.png").c_str());
    }
    if (mvideoButtonPtr) {
        mvideoButtonPtr->setBackgroundPic(
            CONFIGMANAGER->getResFilePath("/HomePage/icon_video_n.png").c_str());
    }
    // 占位背景: 补齐第三页空缺的3个按钮区域
    // 注意: TextViewRightarea2已删除(被videoButton替代)
    if (mTextViewRightarea3Ptr) {
        mTextViewRightarea3Ptr->setBackgroundPic(
                CONFIGMANAGER->getResFilePath("/HomePage/crop_10_rightarea3.png").c_str());
    }
    if (mTextViewRightarea4Ptr) {
        mTextViewRightarea4Ptr->setBackgroundPic(
                CONFIGMANAGER->getResFilePath("/HomePage/crop_11_rightarea4.png").c_str());
    }
    if (mTextViewRightarea5Ptr) {
        mTextViewRightarea5Ptr->setBackgroundPic(
                CONFIGMANAGER->getResFilePath("/HomePage/crop_12_rightarea5.png").c_str());
    }
}

// ============================================================================
// 释放各dock按钮的背景图片 (按页释放)
// ============================================================================
static void release_dock1_button_backgrounds() {
    if (mmusicWindowPtr) mmusicWindowPtr->setBackgroundPic(NULL);
    if (mAudiooutButtonPtr) mAudiooutButtonPtr->setBackgroundPic(NULL);
    if (mCarPlayButtonPtr) mCarPlayButtonPtr->setBackgroundPic(NULL);
    if (mAndroidAutoButtonPtr) mAndroidAutoButtonPtr->setBackgroundPic(NULL);
    if (mBluetoothButtonPtr) mBluetoothButtonPtr->setBackgroundPic(NULL);
}

static void release_dock2_button_backgrounds() {
    if (mAicastButtonPtr) mAicastButtonPtr->setBackgroundPic(NULL);
    if (mMusicButtonPtr) mMusicButtonPtr->setBackgroundPic(NULL);
    if (mSettingsButtonPtr) mSettingsButtonPtr->setBackgroundPic(NULL);
    if (mAirPlayButtonPtr) mAirPlayButtonPtr->setBackgroundPic(NULL);
    if (mMiracastButtonPtr) mMiracastButtonPtr->setBackgroundPic(NULL);
}

static void release_dock3_button_backgrounds() {
    if (mPictureButtonPtr) mPictureButtonPtr->setBackgroundPic(NULL);
    if (mvideoButtonPtr) mvideoButtonPtr->setBackgroundPic(NULL);
    // 释放占位背景 (TextViewRightarea2已删除)
    if (mTextViewRightarea3Ptr) mTextViewRightarea3Ptr->setBackgroundPic(NULL);
    if (mTextViewRightarea4Ptr) mTextViewRightarea4Ptr->setBackgroundPic(NULL);
    if (mTextViewRightarea5Ptr) mTextViewRightarea5Ptr->setBackgroundPic(NULL);
}

// ============================================================================
// 释放所有dock按钮的背景图片 (用于onUI_hide/onUI_quit全量释放)
// ============================================================================
static void release_dock_button_backgrounds() {
    release_dock1_button_backgrounds();
    release_dock2_button_backgrounds();
    release_dock3_button_backgrounds();
}

// ============================================================================
// 设置所有dock按钮的背景图片 (全量，用于初始化等场景)
// ============================================================================
static void set_all_dock_button_backgrounds() {
    set_dock1_button_backgrounds();
    set_dock2_button_backgrounds();
    set_dock3_button_backgrounds();
}

// ============================================================================
// 按页设置dock按钮背景: 只加载当前页，释放其他页，防止图片过多卡顿
// ============================================================================
static void set_dock_button_backgrounds_for_page(int page) {
    switch (page) {
    case 0:
        set_dock1_button_backgrounds();
        release_dock2_button_backgrounds();
        release_dock3_button_backgrounds();
        break;
    case 1:
        release_dock1_button_backgrounds();
        set_dock2_button_backgrounds();
        release_dock3_button_backgrounds();
        break;
    case 2:
        release_dock1_button_backgrounds();
        release_dock2_button_backgrounds();
        set_dock3_button_backgrounds();
        break;
    default:
        set_dock1_button_backgrounds();
        release_dock2_button_backgrounds();
        release_dock3_button_backgrounds();
        break;
    }
}

static void init_default_language() {
    std::string current_code = LANGUAGEMANAGER->getCurrentCode();

    if (current_code.empty() || current_code == "zh_CN") {
        if (!FILE_EXIST("/data/.language_init_flag")) {
            LOGD("First boot detected, setting default language to English");
            sys::setting::update_localescode("en_US");
            system("touch /data/.language_init_flag");
            LOGD("Default language set to English (en_US)");
        } else {
            LOGD("Language init flag exists, current language: %s", current_code.c_str());
        }
    } else {
        LOGD("Current language already set: %s", current_code.c_str());
    }
}

static void check_mcu_auto_upgrade() {
    LOGD("[MAIN] Checking for MCU auto upgrade...");

    if(FILE_EXIST("/mnt/extsd/mcuautoupgrade")){
		EASYUICONTEXT->openActivity("setfactoryActivity");
	}
}

static void _register_timer_fun(int id, int time) {
	mActivityPtr->registerUserTimer(id, time);
}

static void _unregister_timer_fun(int id) {
	mActivityPtr->unregisterUserTimer(id);
}

static void entry_lylink_ftu() {
	if (!sys::reverse_does_enter_status()) {
		EASYUICONTEXT->openActivity("lylinkviewActivity");
		_s_need_reopen_linkview = false;
	} else {
		LOGD("Is reverse status !!!\n");
		lk::video_stop();
		_s_need_reopen_linkview = true;
	}
}

static void _lylink_callback(LYLINKAPI_EVENT evt, int para0, void *para1) {
	switch (evt) {
	case LYLINK_LINK_ESTABLISH:
		LOGD("LYLINK_LINK_ESTABLISH %s", lk::_link_type_to_str((LYLINK_TYPE_E) para0));
		EASYUICONTEXT->hideStatusBar();
		if (LINK_TYPE_AIRPLAY == para0 || LINK_TYPE_MIRACAST == para0 || LINK_TYPE_WIFILY == para0 || LINK_TYPE_WIFICP == para0) {
			if (bt::is_on()) {
				bt::power_off();
			}
			entry_lylink_ftu();
		}
		if(sys::setting::get_sound_mode() == E_SOUND_MODE_LINK){
			if(media::music_is_playing()){
				media::music_pause();
			}
		}
		// 更新连接状态文本
		update_link_status_text();
		break;
	case LYLINK_LINK_DISCONN:
		LOGD("LYLINK_LINK_DISCONN........... %s", lk::_link_type_to_str((LYLINK_TYPE_E) para0));
		if (LINK_TYPE_AIRPLAY == para0 || LINK_TYPE_MIRACAST == para0 || LINK_TYPE_WIFILY == para0 || LINK_TYPE_WIFICP == para0) {
			if (!bt::is_on()) {
				bt::power_on();
			}
		}
		bt::query_state();
		EASYUICONTEXT->closeActivity("lylinkviewActivity");
		// 更新连接状态文本
		update_link_status_text();
		break;
	case LYLINK_PHONE_CONNECT:
		LOGD("LYLINK_PHONE_CONNECT %s", lk::_link_type_to_str((LYLINK_TYPE_E) para0));
		if (para0 == LINK_TYPE_WIFIAUTO || para0 == LINK_TYPE_WIFICP) {
			LOGD("You should open AP now.");
		}
		break;
	case LYLINK_FOREGROUND:
		LOGD("LYLINK_FOREGROUND");
		entry_lylink_ftu();
		break;
	case LYLINK_BACKGROUND:
	case LYLINK_HID_COMMAND:{
		if (evt == LYLINK_BACKGROUND) {
			LOGD("[main] LYLINK_BACKGROUND\n");
		} else {
			LOGD("[main] LYLINK_HID_COMMAND");
		}

		const char *app = EASYUICONTEXT->currentAppName();
		if (app && (strcmp(app, "lylinkviewActivity") == 0)) {
			EASYUICONTEXT->goHome();
		} else {
			EASYUICONTEXT->closeActivity("lylinkviewActivity");
		}
		_s_need_reopen_linkview = false;
	}
		break;
	case LYLINK_PHONE_DISCONN:
		LOGD("LYLINK_PHONE_DISCONN............. %s", lk::_link_type_to_str((LYLINK_TYPE_E) para0));
		lylinkapi_gocsdk("IA\r\n", strlen("IA\r\n"));
		// 更新连接状态文本
		update_link_status_text();
		break;
	default:
		break;
	}
}

static void _reverse_status_cb(int status) {
	LOGD("reverse status %d\n", status);
	base::runInUiThreadUniqueDelayed("rear_view_detection", [](){
		int status = sys::reverse_does_enter_status();
		LOGD("[main] reverse_status %d\n", status);
		if (status == REVERSE_STATUS_ENTER) {
			EASYUICONTEXT->openActivity("reverseActivity");
		} else {
			EASYUICONTEXT->closeActivity("reverseActivity");
			if (_s_need_reopen_linkview) {
				_s_need_reopen_linkview = false;
				if (lk::is_connected()) {
					EASYUICONTEXT->openActivity("lylinkviewActivity");
				}
			}
		}

	}, 50);
}

static void parser() {
	std::string cur_play_file = media::music_get_current_play_file();

	// 如果文件没有改变且已缓存,直接使用缓存
	if (cur_play_file == _last_play_file && _is_music_info_cached && !_cached_title.empty()) {
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setText(_cached_title);
		}
		if (martistTextViewPtr) {
			martistTextViewPtr->setText(_cached_artist);
			martistTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
		}
		return;
	}

	_last_play_file = cur_play_file;

	id3_info_t info;
	memset(&info, 0, sizeof(id3_info_t));
	bool isTrue = media::parse_id3_info(cur_play_file.c_str(), &info);

	// 安全的字符串处理并缓存
	if (isTrue && info.title != nullptr && strlen(info.title) > 0) {
		_cached_title = std::string(info.title);
	} else {
		std::string file_name = fy::files::get_file_name(cur_play_file);
		if (isTrue && !file_name.empty()) {
			_cached_title = file_name;
		} else {
			_cached_title = "Unknown";
		}
	}

	if (isTrue && info.artist != nullptr && strlen(info.artist) > 0) {
		_cached_artist = std::string(info.artist);
	} else {
		_cached_artist = "Unknown";
	}

	// 更新UI
	if (mtitleTextViewPtr) {
		if (_cached_title == "Unknown") {
			mtitleTextViewPtr->setTextTr("Unknown");
		} else {
			mtitleTextViewPtr->setText(_cached_title);
		}
	}

	if (martistTextViewPtr) {
		if (_cached_artist == "Unknown") {
			martistTextViewPtr->setTextTr("Unknown");
		} else {
			martistTextViewPtr->setText(_cached_artist);
		}
		martistTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	}

	_is_music_info_cached = true;
}

// 更新主界面音乐时间
static void update_main_music_time() {
    int curPos = -1;
    int maxPos = -1;

    if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
        if (media::music_get_play_index() != -1) {
            curPos = media::music_get_current_position() / 1000;
            maxPos = media::music_get_duration() / 1000;
        }
    } else if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_BT_MUSIC) {
        bt_music_t music_info = bt::get_music_info();
        curPos = music_info.curpos;
        maxPos = music_info.duration;

        static int last_valid_curPos = -1;
        static int last_valid_maxPos = -1;

        if (curPos >= 0) {
            last_valid_curPos = curPos;
        } else if (last_valid_curPos >= 0) {
            curPos = last_valid_curPos;
        }

        if (maxPos >= 0) {
            last_valid_maxPos = maxPos;
        } else if (last_valid_maxPos >= 0) {
            maxPos = last_valid_maxPos;
        }
    }
}

static void _update_music_info() {
	bt_music_t music_info = bt::get_music_info();
	if (mtitleTextViewPtr) {
		mtitleTextViewPtr->setText(music_info.title);
	}
	if (martistTextViewPtr) {
		martistTextViewPtr->setText(music_info.artist);
		martistTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	}
}

static void _update_music_progress() {
	bt_music_t music_info = bt::get_music_info();

	if (mPlayProgressSeekbarPtr) {
		mPlayProgressSeekbarPtr->setMax(music_info.duration);
		mPlayProgressSeekbarPtr->setProgress(music_info.curpos);
	}
}

static void _bt_music_cb(bt_music_state_e state) {
	// 更新蓝牙连接状态文本
	update_link_status_text();

	// [FIX] 主界面隐藏时跳过UI操作
	if (_is_ui_update_paused) {
		if (bt::music_is_playing()) {
			sys::setting::set_music_play_dev(E_AUDIO_TYPE_BT_MUSIC);
		}
		return;
	}

	if (bt::music_is_playing()) {
		_update_music_info();
		_update_music_progress();
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		}
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(true);
		}
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_BT_MUSIC);
		update_main_music_time();
	} else {
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		}
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(false);
		}
	}
}

static void _music_play_status_cb(music_play_status_e status) {
	// [FIX] 当主界面隐藏时（倒车/互联），仅更新数据状态，不操作UI控件
	// 音乐状态回调可能在任何时刻被media线程触发（通过_notify_play_status_cb），
	// 如果此时main已被hide且正处于倒车摄像头初始化阶段，操作隐藏控件会：
	// 1. 触发SDK内部的texture重分配/刷新，与camera v4l2 buffer争夺内存
	// 2. parse_id3_info() 读取SD卡文件，与camera驱动争夺IO带宽
	// 3. 在FreeMem<2MB时任何malloc都可能导致OOM卡死
	if (_is_ui_update_paused) {
		// 仅保存状态，不操作任何UI
		if (status == E_MUSIC_PLAY_STATUS_STARTED || status == E_MUSIC_PLAY_STATUS_RESUME) {
			sys::setting::set_music_play_dev(E_AUDIO_TYPE_MUSIC);
		}
		// 缓存失效标记，onUI_show时会重新刷新
		_is_music_info_cached = false;
		return;
	}

	switch (status) {
	case E_MUSIC_PLAY_STATUS_STARTED:
		parser();
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_MUSIC);
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(true);
		}
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		}
		if (mPlayProgressSeekbarPtr) {
			mPlayProgressSeekbarPtr->setMax(media::music_get_duration() / 1000);
			mPlayProgressSeekbarPtr->setProgress(0);
		}
		update_main_music_time();
		break;
	case E_MUSIC_PLAY_STATUS_RESUME:
		parser();
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_MUSIC);
		if (mPlayProgressSeekbarPtr) {
			mPlayProgressSeekbarPtr->setMax(media::music_get_duration() / 1000);
		}
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(true);
		}
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		}
		update_main_music_time();
		break;
	case E_MUSIC_PLAY_STATUS_STOP:
		if (mPlayProgressSeekbarPtr) {
			mPlayProgressSeekbarPtr->setMax(media::music_get_duration() / 1000);
		}
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(false);
		}
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
			mtitleTextViewPtr->setTextTr("Unknown");
		}
		if (martistTextViewPtr) {
			martistTextViewPtr->setTextTr("Unknown");
		}
		_is_music_info_cached = false;
		_cached_title.clear();
		_cached_artist.clear();
		_last_play_file.clear();
		break;
	case E_MUSIC_PLAY_STATUS_PAUSE:
		if (mtitleTextViewPtr) {
			mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_NONE);
			mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
		}
		if (mButtonPlayPtr) {
			mButtonPlayPtr->setSelected(false);
		}
		break;
	case E_MUSIC_PLAY_STATUS_COMPLETED:
		LOGE("[main] music play completed, will play next\n");
		media::music_next();
		break;
	case E_MUSIC_PLAY_STATUS_ERROR:
		break;
	}
}

static void _bt_call_cb(bt_call_state_e state) {
	// 更新蓝牙连接状态文本
	update_link_status_text();

	if (state != E_BT_CALL_STATE_IDLE) {
		if (lk::get_lylink_type() == LINK_TYPE_WIFIAUTO) {
			const char *app = EASYUICONTEXT->currentAppName();
			if (!app) return;
			if(strcmp(app, "reverseActivity") == 0) {
				_s_need_reopen_linkview = true;
			} else if(strcmp(app, "lylinkviewActivity") != 0) {
				EASYUICONTEXT->openActivity("lylinkviewActivity");
			}
		}
	}
}

static void _bt_add_cb() {
	_s_bt_cb.call_cb = _bt_call_cb;
	_s_bt_cb.music_cb = _bt_music_cb;
	bt::add_cb(&_s_bt_cb);
}

static void _bt_remove_cb() {
	bt::remove_cb(&_s_bt_cb);
}

static bool _show_sys_info(unsigned long *freeram) {
	struct sysinfo info;
	int ret = 0;
	ret = sysinfo(&info);
	if(ret != 0) {
		return false;
	}
	*freeram = info.freeram;
	return true;
}

static bool is_system_time_valid() {
    time_t now = time(nullptr);
    struct tm *t = localtime(&now);
    return (t->tm_year + 1900 >= 2020) || FILE_EXIST(TIME_SYNCED_FLAG);
}

static void ctrl_UI_init() {
	EASYUICONTEXT->hideStatusBar();
}

static void set_back_pic() {
}

// ============================================================================
// 设置分片背景图片 (替代原来的单一 TextViewBg)
// 8个控件分别对应8张裁剪后的背景图片
// ============================================================================
static void set_split_backgrounds() {
    bitmap_t *bmp = NULL;
    MEM_SNAP_SIMPLE("main_split_bg_START");

    // 1. Top
    if (mTextViewTopPtr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_top);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_1_top.jpg").c_str(), 3);
        mTextViewTopPtr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_top", img_top, CONFIGMANAGER->getResFilePath("/HomePage/crop_1_top.jpg").c_str());
    }

    // 2. Bottom
    if (mTextViewBottomPtr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_bottom);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_2_bottom.jpg").c_str(), 3);
        mTextViewBottomPtr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_bottom", img_bottom, CONFIGMANAGER->getResFilePath("/HomePage/crop_2_bottom.jpg").c_str());
    }

    // 3. Left
    if (mTextViewLeftPtr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_left);
//        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_3_left.jpg").c_str(), 3);
        mTextViewLeftPtr->setBackgroundPic(CONFIGMANAGER->getResFilePath("/HomePage/crop_3_left.png").c_str());
        MEM_IMG_LOAD_END("split_bg_left", img_left, CONFIGMANAGER->getResFilePath("/HomePage/crop_3_left.png").c_str());
    }

    // 4. Divider1
    if (mTextViewDivider1Ptr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_div1);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_4_divider1.jpg").c_str(), 3);
        mTextViewDivider1Ptr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_divider1", img_div1, CONFIGMANAGER->getResFilePath("/HomePage/crop_4_divider1.jpg").c_str());
    }

    // 5. Divider2
    if (mTextViewDivider2Ptr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_div2);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_5_divider2.jpg").c_str(), 3);
        mTextViewDivider2Ptr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_divider2", img_div2, CONFIGMANAGER->getResFilePath("/HomePage/crop_5_divider2.jpg").c_str());
    }

    // 6. Divider3
    if (mTextViewDivider3Ptr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_div3);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_6_divider3.jpg").c_str(), 3);
        mTextViewDivider3Ptr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_divider3", img_div3, CONFIGMANAGER->getResFilePath("/HomePage/crop_6_divider3.jpg").c_str());
    }

    // 7. Divider4
    if (mTextViewDivider4Ptr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_div4);
        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_7_divider4.jpg").c_str(), 3);
        mTextViewDivider4Ptr->setBackgroundBmp(bmp);
        MEM_IMG_LOAD_END("split_bg_divider4", img_div4, CONFIGMANAGER->getResFilePath("/HomePage/crop_7_divider4.jpg").c_str());
    }

    // 8. Right
    if (mTextViewRightPtr) {
        bmp = NULL;
        MEM_IMG_LOAD_BEGIN(img_right);
//        BitmapHelper::loadBitmapFromFile(bmp, CONFIGMANAGER->getResFilePath("/HomePage/crop_8_right.jpg").c_str(), 3);
        mTextViewRightPtr->setBackgroundPic(CONFIGMANAGER->getResFilePath("/HomePage/crop_8_right.png").c_str());
        MEM_IMG_LOAD_END("split_bg_right", img_right, CONFIGMANAGER->getResFilePath("/HomePage/crop_8_right.png").c_str());
    }

    MEM_SNAP_SIMPLE("main_split_bg_END");
}

// ============================================================================
// 释放分片背景图片资源
// ============================================================================
static void release_split_backgrounds() {
    if (mTextViewTopPtr) mTextViewTopPtr->setBackgroundBmp(NULL);
    if (mTextViewBottomPtr) mTextViewBottomPtr->setBackgroundBmp(NULL);
    if (mTextViewLeftPtr) mTextViewLeftPtr->setBackgroundBmp(NULL);
    if (mTextViewDivider1Ptr) mTextViewDivider1Ptr->setBackgroundBmp(NULL);
    if (mTextViewDivider2Ptr) mTextViewDivider2Ptr->setBackgroundBmp(NULL);
    if (mTextViewDivider3Ptr) mTextViewDivider3Ptr->setBackgroundBmp(NULL);
    if (mTextViewDivider4Ptr) mTextViewDivider4Ptr->setBackgroundBmp(NULL);
    if (mTextViewRightPtr) mTextViewRightPtr->setBackgroundBmp(NULL);
}

// 更新所有控件的背景图片
static void update_all_backgrounds_for_mode() {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    LOGD("[main] update_all_backgrounds_for_mode() ENTER");

    // 1. 主背景 - 使用8个分片控件替代原来的单一 TextViewBg
    LOGD("[main] Setting day mode split backgrounds");
    set_split_backgrounds();

    // 2. music progress background
    if (mPlayProgressSeekbarPtr && mTextView2Ptr) {
        mTextView2Ptr->setBackgroundPic(CONFIGMANAGER->getResFilePath("/HomePage/progress_n.png").c_str());
    }

    // 3. 只设置当前页dock按钮的背景图片，防止图片过多卡顿
    set_dock_button_backgrounds_for_page(_current_page_index);

    LOGD("[main] All backgrounds updated for day mode");

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    LOGD("[main] update_all_backgrounds_for_mode() EXIT, elapsed time: %.3f seconds", elapsed_time);
}

static void _preload_resources() {
	const char *pic_tab[] = {
		"/res/font/sanswithfz_jp_it_pt.ttf",
		"navi/bg_bt_n.png",
		"navi/bg_bt_p.png",
		"navi/bg_eq_n.png",
		"navi/bg_eq_p.png",
		"navi/bg_fm_n.png",
		"navi/bg_fm_p.png",
		"navi/bg_screen_off_n.png",
		"navi/bg_screen_off_p.png",
		"navi/icon_btvoice.png",
		"navi/icon_light.png",
		"navi/icon_setting_n.png",
		"navi/icon_setting_p.png",
		"navi/icon_voice.png",
		"navi/progress_n.png",
		"navi/progress_p.png",
	};

	LOGD("[main] preload resources start\n");

	size_t size = TAB_SIZE(pic_tab);
	for (size_t i = 0; i < size; ++i) {
		if (i == 0) {
			fy::cache_file(pic_tab[i]);
		} else {
			fy::cache_file(CONFIGMANAGER->getResFilePath(pic_tab[i]));
		}
	}

	LOGD("[main] preload resources end\n");
}

static void key_status(bool down) {
	static bool is_down = false;
	static uint32_t mDownTime;
	mActivityPtr->unregisterUserTimer(TIMER_POWERKEY_EVENT);
	if (down) {
		if (!is_down) {
			is_down = true;
			mActivityPtr->registerUserTimer(TIMER_POWERKEY_OFF, 1000);
			return ;
		}
	} else {
		if (is_down) {
			if (sys::reverse_does_enter_status()) {
				LOGD("[main] is reverse status, don't proc screensaver\n");
				return ;
			}
			mActivityPtr->registerUserTimer(TIMER_POWERKEY_EVENT, 100);
		}
		is_down = false;
		mActivityPtr->unregisterUserTimer(TIMER_POWERKEY_OFF);
		key_sec = 0;
	}
}

static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	{1,  1000},
	{QUERY_LINK_AUTH_TIMER, 6000},
	{SWITCH_ADB_TIMER, 1000},
};

static void onUI_init() {
	MEM_LIFECYCLE("main", "init");
	MEM_VM_DETAIL("main_init_ENTRY");
	_preload_resources();
	MEM_SNAP_SIMPLE("main_init_after_preload");
	ctrl_UI_init();

    _is_in_reverse_mode = false;
    _is_ui_update_paused = false;
    _is_music_info_cached = false;
    _background_resources_loaded = false;
    _is_exiting_reverse = false;
    _cached_title.clear();
    _cached_artist.clear();
    _last_play_file.clear();
    _current_page_index = 0;

	sys::setting::init();
	sys::hw::init();

	uart::init();
	uart::set_amplifier_mute(0);
	uart::set_power_cr(1);
	uart::add_power_state_cb(key_status);

	bt::init();
	mActivityPtr->registerUserTimer(BT_MUSIC_ID3, 0);

	net::init();

	media::init();
	media::music_add_play_status_cb(_music_play_status_cb);

	lk::add_lylink_callback(_lylink_callback);
	lk::start_lylink();

	app::attach_timer(_register_timer_fun, _unregister_timer_fun);

	sys::reverse_add_status_cb(_reverse_status_cb);
	sys::reverse_detect_start();

	_bt_add_cb();
	bt::query_state();

	// [FIX] 删除重复注册: music_add_play_status_cb 已在上方注册，重复注册导致
	// _music_play_status_cb 在音乐状态变化时被调用两次，每次都执行 parser() 和 UI更新，
	// 在低内存时双倍开销可能成为压垮骆驼的最后一根稻草
	if (martistTextViewPtr) {
		martistTextViewPtr->setTouchPass(true);
	}

	// 初始化RadioGroup状态
	if (mStatusRadioGroupPtr) {
		mStatusRadioGroupPtr->setCheckedID(ID_MAIN_RadioButton0);
	}

	// 初始化页面显示 - 默认显示第一页
	switch_dock_window(0);

	if(bt::is_calling()){
		bt::call_vol(audio::get_lylink_call_vol());
	}

	base::UiHandler::implementTimerRegistration([]() {
		mActivityPtr->registerUserTimer(base::TIMER_UI_HANDLER, 0);
	});
}

static void onUI_intent(const Intent *intentPtr) {
    if (intentPtr != NULL) {
    }
}

static void onUI_show() {
	MEM_LIFECYCLE("main", "show");
	mode::set_switch_mode(E_SWITCH_MODE_NULL);
    LOGD("[main] Transitioning setup gannina");
    _is_ui_update_paused = false;
    _is_in_reverse_mode = false;

    // [FIX] 回到主界面时无条件恢复下拉功能
    // 问题: lylinkview onUI_show 设 setCanSlide(false) 禁止下拉，
    //   用户通过floatwnd home回到main后，虽然lylinkview hide中恢复了setCanSlide(true)，
    //   但hide_topbar/show_topbar是异步timer，fold_statusbar()中的setCanSlide(false)
    //   可能在lylinkview hide之后才执行，导致从main进入其他Activity(如图片浏览)时
    //   下拉仍被禁用。在main onUI_show中无条件恢复是最可靠的兜底
    SLIDEMANAGER->setCanSlide(true);

	int curPos = -1;

    // 更新所有背景(包括分片主背景和所有控件)
    update_all_backgrounds_for_mode();

	if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_BT_MUSIC) {
		_update_music_info();
		_update_music_progress();
		if (bt::music_is_playing()) {
			if (mButtonPlayPtr) {
				mButtonPlayPtr->setSelected(true);
			}
		} else {
			// [FIX-A] 蓝牙音乐未在播放时，确保按钮显示为暂停状态
			// 场景: 进入CarPlay/互联后蓝牙音乐被暂停，退出回mainLogic时
			// onUI_hide期间_is_ui_update_paused=true导致pause回调未更新按钮
			if (mButtonPlayPtr) {
				mButtonPlayPtr->setSelected(false);
			}
		}
		update_main_music_time();
	} else if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
		parser();
		if (media::music_is_playing()) {
			if (mButtonPlayPtr) {
				mButtonPlayPtr->setSelected(true);
			}
			curPos = media::music_get_current_position() / 1000;
			if (mPlayProgressSeekbarPtr) {
				mPlayProgressSeekbarPtr->setMax(media::music_get_duration() / 1000);
			}
			if (mtitleTextViewPtr) {
				mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
				mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
			}
		} else {
			// [FIX-A] 本地音乐未在播放时，确保按钮显示为暂停状态
			// 场景: 播放本地音乐 → 进入CarPlay(音乐被pause) → 退出CarPlay回main
			// 由于进入CarPlay时onUI_hide设置_is_ui_update_paused=true，
			// music_pause的回调在_music_play_status_cb中被提前return跳过了，
			// 导致ButtonPlay仍残留为selected(true)即播放状态
			if (mButtonPlayPtr) {
				mButtonPlayPtr->setSelected(false);
			}
			if (mtitleTextViewPtr) {
				mtitleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_NONE);
				mtitleTextViewPtr->setTextColor(0xFFFFFFFF);
			}
			// 音乐暂停但有播放记录时，仍然显示当前进度
			if (media::music_get_play_index() != -1) {
				curPos = media::music_get_current_position() / 1000;
				if (mPlayProgressSeekbarPtr) {
					mPlayProgressSeekbarPtr->setMax(media::music_get_duration() / 1000);
				}
			}
		}
		update_main_music_time();
	}
    if (curPos >= 0) {
    	if (mPlayProgressSeekbarPtr) {
    		mPlayProgressSeekbarPtr->setProgress(curPos);
    	}
    }
	if (!app::is_show_topbar()) {
		sys::setting::set_reverse_topbar_show(true);
		app::show_topbar();
	}
	init_default_language();
	check_mcu_auto_upgrade();

	if (mmusictextPtr) {
		mmusictextPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	}

	// 恢复页面可见性状态
	switch_dock_window(_current_page_index);

	// 更新CarPlay和AndroidAuto连接状态
	update_link_status_text();
}

static void onUI_hide() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	LOGD("[main] onUI_hide() ENTER");

	LOGD("[main] onUI_hide - cleaning up resources");
	MEM_LIFECYCLE("main", "hide");
	_is_ui_update_paused = true;

	// 释放分片背景资源
	release_split_backgrounds();

	if (mToMusicPtr) {
		mToMusicPtr->setBackgroundPic(NULL);
	}

	// 释放dock按钮背景
	release_dock_button_backgrounds();

	_is_music_info_cached = false;
	_cached_title.clear();
	_cached_artist.clear();

	// [FIX] 释放完背景后立即drop_caches，在camera startPreview之前
	// 腾出pagecache空间给v4l2 mmap buffer
	fy::drop_caches();

	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
	                      (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
	LOGD("[main] onUI_hide() EXIT, elapsed time: %.3f seconds", elapsed_time);
}

static void onUI_quit() {
	MEM_LIFECYCLE("main", "quit");
	uart::remove_power_state_cb(key_status);
	lk::remove_lylink_callback(_lylink_callback);
	media::music_remove_play_status_cb(_music_play_status_cb);
	if (mPlayProgressSeekbarPtr) {
		mPlayProgressSeekbarPtr->setSeekBarChangeListener(NULL);
	}

	// 释放分片背景资源
	release_split_backgrounds();

	if (mToMusicPtr) {
		mToMusicPtr->setBackgroundPic(NULL);
	}

	// 释放dock按钮背景
	release_dock_button_backgrounds();

	_is_music_info_cached = false;
	_cached_title.clear();
	_cached_artist.clear();
	_last_play_file.clear();

	_bt_remove_cb();
}

static void onProtocolDataUpdate(const SProtocolData &data) {

}

static bool onUI_Timer(int id) {
	if (app::on_timer(id)) {
		return false;
	}
	switch (id) {
	case 0: {
		unsigned long freeram = 0;
		bool ret = _show_sys_info(&freeram);
		if(ret) {
			LOGD("-----------Current MemFree: %ldKB---------------", freeram >> 10);
		} else {
			LOGD("-----------get MemFree info fail----------------");
		}
	}
		break;
	case 1: {
        MEM_TIMER_SNAP_SAMPLED("main_1s", 1, 10);
        MEM_WARN_IF_LOW("main_timer", 4000);

        // [FIX] 当主界面被hide时（倒车/互联/其他Activity覆盖），跳过UI更新
        // 避免在隐藏状态下操作SeekBar等控件，减少不必要的CPU和内存开销
        if (_is_ui_update_paused) {
            break;
        }

        int curPos = -1;
        if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
            if (media::music_is_playing()) {
                curPos = media::music_get_current_position() / 1000;
            }
            if (curPos >= 0) {
            	if (mPlayProgressSeekbarPtr) {
            		mPlayProgressSeekbarPtr->setProgress(curPos);
            	}
            }
        }
        update_main_music_time();
	}
		break;
	case QUERY_LINK_AUTH_TIMER:
		lk::query_is_authorized();
		return false;
	case SWITCH_ADB_TIMER: {
		if (strcmp("UpgradeActivity", EASYUICONTEXT->currentAppName()) == 0) {
			app::hide_topbar();
		}
		if (sys::setting::is_usb_adb_enabled()) {
			if (sys::get_usb_mode() != E_USB_MODE_DEVICE) {
				sys::change_usb_mode(E_USB_MODE_DEVICE);
			}
		} else {
			sys::set_usb_config(E_USB_MODE_HOST);
		}
		uart::set_amplifier_mute(1);
	}
		return false;
	case BT_MUSIC_ID3:
		if (bt::music_is_playing()) {
			bt::query_music_info();
		}
		return false;
	case MUSIC_ERROR_TIMER:
		media::music_next(true);
		return false;
    case base::TIMER_UI_HANDLER:
      return base::UiHandler::onTimer();
      break;
	case TIMER_POWERKEY_EVENT:
		if (strcmp("screenOffActivity", EASYUICONTEXT->currentAppName()) == 0) {
			EASYUICONTEXT->closeActivity("screenOffActivity");
		} else {
			fold_statusbar();
	    	EASYUICONTEXT->openActivity("screenOffActivity");
		}
		return false;
	case TIMER_POWERKEY_OFF:{
		if ((key_sec++) >= 2) {
			if (sys::reverse_does_enter_status()) {
				LOGD("[main] is reverse status, don't proc key long press\n");
				key_sec = 0;
				return false;
			}
			if (lk::is_connected()) {
				lk::stop_lylink();
			}
			uart::app_turn_off();
		}
	}
		break;
	default:
		break;
	}
    return true;
}

/**
 * 检查触摸点是否在SeekBar区域内
 * 用于防止在操作SeekBar时误触发窗口切换
 */
static bool isTouchInSeekBarArea(int x, int y) {
	int margin = 20;  // 扩大检测区域，增加容错范围（上下左右各扩展20像素）

	// 检查播放进度SeekBar区域
	if (mPlayProgressSeekbarPtr && mPlayProgressSeekbarPtr->isVisible()) {
		LayoutPosition playPos = mPlayProgressSeekbarPtr->getPosition();
		if (x >= (playPos.mLeft - margin) && x <= (playPos.mLeft + playPos.mWidth + margin) &&
		    y >= (playPos.mTop - margin) && y <= (playPos.mTop + playPos.mHeight + margin)) {
			return true;
		}
	}

	// 检查ctrlbar是否显示，如果显示则检查触摸点是否在ctrlbar区域内
	// ctrlbar包含音量和亮度SeekBar，当它显示时禁止主界面的滑动切换
	if (app::is_show_ctrlbar()) {
		return true;
	}

	return false;
}

// ============================================================================
// 触摸事件处理 - 使用手指移动距离判断翻页
// ============================================================================
static bool onmainActivityTouchEvent(const MotionEvent &ev) {
	LayoutPosition pos = EASYUICONTEXT->getNaviBar()->getPosition();

	static MotionEvent down_ev;
	static bool allow_switch;
	static bool is_seekbar_touch;  // 标记是否在SeekBar区域触摸

	if (pos.mTop != -pos.mHeight) { return false; }

	switch (ev.mActionStatus) {
	case MotionEvent::E_ACTION_DOWN:
		// 检查触摸起始点是否在SeekBar区域
		is_seekbar_touch = isTouchInSeekBarArea(ev.mX, ev.mY);
		if (is_seekbar_touch) {
			// 如果在SeekBar区域，不允许切换窗口
			allow_switch = false;
			LOGD("[main] Touch started in SeekBar area, disable window switch");
		} else {
			allow_switch = true;
		}
		down_ev = ev;
		break;
	case MotionEvent::E_ACTION_MOVE:
		// 如果正在SeekBar区域操作，继续禁止窗口切换
		if (is_seekbar_touch) {
			allow_switch = false;
		}
		break;
	case MotionEvent::E_ACTION_UP:
		// 滑动距离阈值: SCREEN_WIDTH / 10 (约160像素)
	    if (allow_switch && !is_seekbar_touch && (abs(ev.mX - down_ev.mX) >= SCREEN_WIDTH / 10)) {
	        int delta_x = ev.mX - down_ev.mX;
	        LOGD("[main] Swipe detected, delta_x = %d, current_page = %d", delta_x, _current_page_index);

	        // 边界检查: 防止在边界页面进行无效滑动
	        bool allow_change = true;
	        if (_current_page_index == 0 && delta_x > 0) {
	            // 第一页向右滑动无效
	            allow_change = false;
	            LOGD("[main] Already at first page, cannot swipe right");
	        } else if (_current_page_index == 2 && delta_x < 0) {
	            // 最后一页向左滑动无效
	            allow_change = false;
	            LOGD("[main] Already at last page, cannot swipe left");
	        }

	        if (allow_change) {
	            switch_page_by_direction(delta_x);
	        }
	    } else {
	        LOGD("[main] Swipe not triggered: allow_switch=%d, is_seekbar_touch=%d, distance=%d, threshold=%d",
	             allow_switch, is_seekbar_touch, abs(ev.mX - down_ev.mX), SCREEN_WIDTH / 10);
	    }

	    allow_switch = false;
	    is_seekbar_touch = false;  // 重置SeekBar触摸标记
		break;
	default:
		break;
	}
	return false;
}

static bool onButtonClick_NextButton(ZKButton *pButton) {
    LOGD(" ButtonClick NextButton !!!\n");
	if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return false;
	}
	if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
	    media::music_next(true);
	} else if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_BT_MUSIC) {
		bt::music_next();
	}
    return false;
}

static bool onButtonClick_ButtonPlay(ZKButton *pButton) {
    LOGD(" ButtonClick ButtonPlay !!!\n");

	if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return false;
	}

	if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
	    if (media::music_get_play_index() == -1) {
	    	return false;
	    } else if (media::music_is_playing()) {
	        media::music_pause();
	    } else {
	    	media::music_resume();
	    }
	} else if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_BT_MUSIC) {
	    bt::music_is_playing() ? bt::music_pause() : bt::music_play();
	}
    return false;
}

static bool onButtonClick_PrevButton(ZKButton *pButton) {
    LOGD(" ButtonClick PrevButton !!!\n");
	if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return false;
	}
	if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC) {
	    media::music_prev(true);
	} else if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_BT_MUSIC) {
		bt::music_prev();
	}
    return false;
}

static void onProgressChanged_PlayProgressSeekbar(ZKSeekBar *pSeekBar, int progress) {
}

static bool onButtonClick_Setting(ZKButton *pButton) {
    LOGD(" ButtonClick Setting !!!\n");
    EASYUICONTEXT->openActivity("settingsActivity");
    return false;
}

static bool onButtonClick_ToMusic(ZKButton *pButton) {
    LOGD(" ButtonClick ToMusic !!!\n");
	if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return false;
	}
    return false;
}

static void open_linkhelp_activity(link_mode_e mode) {
	Intent *i = new Intent;
	i->putExtra("link_mode", fy::format("%d", mode));
	LOGD("[main] choose link mode %s\n", sys::setting::get_link_mode_str(mode));
	EASYUICONTEXT->openActivity("linkhelpActivity", i);
}

static void _change_link_app(link_mode_e mode) {
	switch (mode) {
	case E_LINK_MODE_HICAR:
	case E_LINK_MODE_ANDROIDAUTO:
	case E_LINK_MODE_CARPLAY:
		if (net::get_mode() != E_NET_MODE_AP) { net::change_mode(E_NET_MODE_AP);}
		break;
	case E_LINK_MODE_AIRPLAY:
		if (net::get_mode() != E_NET_MODE_AP) { net::change_mode(E_NET_MODE_AP); }
		break;
	case E_LINK_MODE_CARLIFE:
		if (net::get_mode() != E_NET_MODE_WIFI) { net::change_mode(E_NET_MODE_WIFI); }
		break;
	case E_LINK_MODE_MIRACAST:
	case E_LINK_MODE_LYLINK:
		if (net::get_mode() != E_NET_MODE_P2P) { net::change_mode(E_NET_MODE_P2P); }
		break;
	default:
		break;
	}
	open_linkhelp_activity(mode);
}

static void open_link_activity(link_mode_e mode) {
	LYLINK_TYPE_E link_type = lk::get_lylink_type();
	switch(mode) {
	case E_LINK_MODE_CARPLAY:
		if ((link_type == LINK_TYPE_WIFICP) || (link_type == LINK_TYPE_USBCP)) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_ANDROIDAUTO:
		if ((link_type == LINK_TYPE_WIFIAUTO) || (link_type == LINK_TYPE_USBAUTO)) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_CARLIFE:
		if ((link_type == LINK_TYPE_WIFILIFE) || (link_type == LINK_TYPE_USBLIFE)) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_HICAR:
		if ((link_type == LINK_TYPE_WIFIHICAR) || (link_type == LINK_TYPE_USBHICAR)) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_MIRACAST:
		if (link_type == LINK_TYPE_MIRACAST) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_LYLINK:
		if (link_type == LINK_TYPE_WIFILY) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	case E_LINK_MODE_AIRPLAY:
		if (link_type == LINK_TYPE_AIRPLAY) {
			EASYUICONTEXT->openActivity("lylinkviewActivity");
			return;
		}
		break;
	default:
		break;
	}
	if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return;
	}

	if (mode == E_LINK_MODE_AIRPLAY || mode == E_LINK_MODE_LYLINK || mode ==E_LINK_MODE_MIRACAST) {
		if (bt::is_on()) {
			bt::power_off();
		}
	} else {
		if (!bt::is_on()) {
			bt::power_on();
		}
	}

	open_linkhelp_activity(mode);
}

static bool onButtonClick_Button1(ZKButton *pButton) {
    LOGD(" ButtonClick Button1 !!!\n");
    EASYUICONTEXT->openActivity("mcubtUpdActivity");
    return false;
}

static bool onButtonClick_Button2(ZKButton *pButton) {
    LOGD(" ButtonClick Button2 !!!\n");
    EASYUICONTEXT->openActivity("soundEffectActivity");
    return false;
}

static void onCheckedChanged_StatusRadioGroup(ZKRadioGroup* pRadioGroup, int checkedID) {
    LOGD(" RadioGroup StatusRadioGroup checked %d", checkedID);

    int page = 0;
    switch (checkedID) {
    case ID_MAIN_RadioButton0:
        page = 0;
        break;
    case ID_MAIN_RadioButton1:
        page = 1;
        break;
    case ID_MAIN_RadioButton2:
        page = 2;
        break;
    default:
        page = 0;
        break;
    }

    // 切换到对应页面
    switch_dock_window(page);
}

static void onProgressChanged_PlayVolSeekBar(ZKSeekBar *pSeekBar, int progress) {
}

static bool onButtonClick_button_apps(ZKButton *pButton) {
    LOGD(" ButtonClick button_apps !!!\n");
    return false;
}

static void onCheckedChanged_RadioGroup1(ZKRadioGroup* pRadioGroup, int checkedID) {
    LOGD(" RadioGroup RadioGroup1 checked %d", checkedID);
}

static void onCheckedChanged_page3RadioGroup(ZKRadioGroup* pRadioGroup, int checkedID) {
    LOGD(" RadioGroup page3RadioGroup checked %d", checkedID);
}

static bool onButtonClick_audiooutput3Button(ZKButton *pButton) {
    LOGD(" ButtonClick audiooutput3Button !!!\n");
    return false;
}

static bool onButtonClick_toLocalmusicButton(ZKButton *pButton) {
    LOGD(" ButtonClick toLocalmusicButton !!!\n");
    if (lk::is_connected()) {
		if (mlinkTipsWindowPtr) {
			mlinkTipsWindowPtr->showWnd();
		}
		return false;
	}
    // 只有当音源为本地音乐时才跳转到musicActivity
    if (sys::setting::get_music_play_dev() == E_AUDIO_TYPE_MUSIC || bt::get_connect_state() != E_BT_CONNECT_STATE_CONNECTED) {
        EASYUICONTEXT->openActivity("musicActivity");
    }
    return false;
}

// ============================================================================
// dock1Window 按钮点击事件
// dock1: AudiooutButton, CarPlayButton, AndroidAutoButton, BluetoothButton
// ============================================================================
static bool onButtonClick_AudiooutButton(ZKButton *pButton) {
    LOGD(" ButtonClick AudiooutButton !!!\n");
    EASYUICONTEXT->openActivity("FMemitActivity");
    return false;
}

static bool onButtonClick_CarPlayButton(ZKButton *pButton) {
    LOGD(" ButtonClick CarPlayButton !!!\n");
    open_link_activity(E_LINK_MODE_CARPLAY);
    return false;
}

static bool onButtonClick_AndroidAutoButton(ZKButton *pButton) {
    LOGD(" ButtonClick AndroidAutoButton !!!\n");
    open_link_activity(E_LINK_MODE_ANDROIDAUTO);
    return false;
}

static bool onButtonClick_BluetoothButton(ZKButton *pButton) {
    LOGD(" ButtonClick BluetoothButton !!!\n");
    if (lk::is_connected()) {
        if (mlinkTipsWindowPtr) {
            mlinkTipsWindowPtr->showWnd();
        }
        return false;
    }
    EASYUICONTEXT->openActivity("btsettingActivity");
    return false;
}

// ============================================================================
// dock2Window 按钮点击事件
// dock2: AicastButton, MusicButton, SettingsButton, AirPlayButton, MiracastButton
// ============================================================================
static bool onButtonClick_AicastButton(ZKButton *pButton) {
    LOGD(" ButtonClick AicastButton !!!\n");
    open_link_activity(E_LINK_MODE_LYLINK);
    return false;
}

static bool onButtonClick_MusicButton(ZKButton *pButton) {
    LOGD(" ButtonClick MusicButton !!!\n");
    if (lk::is_connected()) {
        if (mlinkTipsWindowPtr) {
            mlinkTipsWindowPtr->showWnd();
        }
        return false;
    }
    EASYUICONTEXT->openActivity("musicActivity");
    return false;
}

static bool onButtonClick_SettingsButton(ZKButton *pButton) {
    LOGD(" ButtonClick SettingsButton !!!\n");
    EASYUICONTEXT->openActivity("setshowActivity");
    return false;
}

static bool onButtonClick_AirPlayButton(ZKButton *pButton) {
    LOGD(" ButtonClick AirPlayButton !!!\n");
    open_link_activity(E_LINK_MODE_AIRPLAY);
    return false;
}

static bool onButtonClick_MiracastButton(ZKButton *pButton) {
    LOGD(" ButtonClick MiracastButton !!!\n");
    open_link_activity(E_LINK_MODE_MIRACAST);
    return false;
}

// ============================================================================
// dock3Window 按钮点击事件
// dock3: PictureButton, videoButton
// ============================================================================
static bool onButtonClick_PictureButton(ZKButton *pButton) {
    LOGD(" ButtonClick PictureButton !!!\n");
    if (lk::is_connected()) {
        if (mlinkTipsWindowPtr) {
            mlinkTipsWindowPtr->showWnd();
        }
        return false;
    }
    EASYUICONTEXT->openActivity("PhotoAlbumActivity");
    return false;
}

static bool onButtonClick_videoButton(ZKButton *pButton) {
    LOGD(" ButtonClick videoButton !!!\n");
    if (lk::is_connected()) {
        if (mlinkTipsWindowPtr) {
            mlinkTipsWindowPtr->showWnd();
        }
        return false;
    }
    EASYUICONTEXT->openActivity("videoActivity");
    return false;
}