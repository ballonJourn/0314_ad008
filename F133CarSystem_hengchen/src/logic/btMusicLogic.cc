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

#include "bt/context.h"
#include "link/context.h"
#include "system/setting.h"
#include "utils/BitmapHelper.h"
#include "manager/ConfigManager.h"
#include "mode_observer.h"
#include "media/audio_context.h"  // 添加音频上下文头文件

extern void set_navibar_PlayVolSeekBar(int progress);

static bt_cb_t _s_bt_cb;

static std::string _time_format(int time) {
	char buf[32] = { 0 };
	sprintf(buf, "%02d:%02d", time/60, time%60);
	return buf;
}

static void _update_music_info() {
	bt_music_t music_info = bt::get_music_info();
//	mtittleTextViewPtr->setText(music_info.title);
//	martistTextViewPtr->setText(music_info.artist);
//	malbumTextViewPtr->setText(music_info.album);
	(strcmp(music_info.title.c_str(), "") == 0) ? mtittleTextViewPtr->setTextTr("Unknown") : mtittleTextViewPtr->setText(music_info.title);
	(strcmp(music_info.artist.c_str(), "") == 0) ? martistTextViewPtr->setTextTr("Unknown") : martistTextViewPtr->setText(music_info.artist);
	(strcmp(music_info.album.c_str(), "") == 0) ? malbumTextViewPtr->setTextTr("Unknown") : malbumTextViewPtr->setText(music_info.album);
//	mlyricTextViewPtr->setText(music_info.lyric);
}

static void _update_music_progress() {
	bt_music_t music_info = bt::get_music_info();

	mmusicSeekBarPtr->setMax(music_info.duration);
	mmusicSeekBarPtr->setProgress(music_info.curpos);
	mcurposTextViewPtr->setText(_time_format(music_info.curpos));
	mdurationTextViewPtr->setText(_time_format(music_info.duration));
}


static void _bt_music_cb(bt_music_state_e state) {
	if (bt::music_is_playing()) {
		_update_music_info();
		_update_music_progress();
		mtittleTextViewPtr->setTextColor(0xFF00FCFF);
		mplayButtonPtr->setSelected(true);
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_BT_MUSIC);
	} else {
		mtittleTextViewPtr->setTextColor(0xFFFFFFFF);
		mplayButtonPtr->setSelected(false);
	}
}

static void _bt_add_cb() {
	_s_bt_cb.music_cb = _bt_music_cb;
	bt::add_cb(&_s_bt_cb);
}

static void _bt_remove_cb() {
	bt::remove_cb(&_s_bt_cb);
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
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	LOGD("[btMusic] onUI_init");
	_bt_add_cb();

	// 确保音源切换到蓝牙音乐
	LOGD("[btMusic] Switch audio to BT_MUSIC");
//	audio::change_audio_type(E_AUDIO_TYPE_BT_MUSIC);

	if (bt::music_is_playing()) {
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_BT_MUSIC);
	}




	float current_vol = audio::get_system_vol();

	// 找到当前音量对应的步进索引 (0, 0.1, 0.2, ..., 1.6)
	int current_step = (int)((current_vol / 0.1f) + 0.5f);  // 四舍五入到最近的步进
	current_step = (current_step < 0) ? 0 : (current_step > 16) ? 16 : current_step;

	// 计算临时音量 (增加或减少一个步进)
	int temp_step = current_step + 1;
	if (temp_step > 16) {
		temp_step = current_step - 1;
		if (temp_step < 0) {
			temp_step = 0;
		}
	}
	float temp_vol = temp_step * 0.1f;  // 确保按 0.1 的步进


//	audio::set_system_vol(temp_vol, true);   // 先调整
	set_navibar_PlayVolSeekBar(temp_step);
	DELAY(50);  // 等待生效
//	audio::set_system_vol(current_vol, true); // 恢复原音量
	set_navibar_PlayVolSeekBar(current_step);
	DELAY(50);  // 等待生效



	mplayButtonPtr->setSelected(bt::music_is_playing());
	mtittleTextViewPtr->setTextColor(bt::music_is_playing() ? 0xFF00FCFF : 0xFFFFFFFF);
	_update_music_info();		// 音乐信息
	_update_music_progress();	// 进度信息
	mtittleTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	martistTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	malbumTextViewPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
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
	mode::set_switch_mode(E_SWITCH_MODE_NULL);

	// 如果蓝牙音乐正在播放但UI状态不对，修正UI状态
	if (bt::music_is_playing()) {
		mplayButtonPtr->setSelected(true);
		mtittleTextViewPtr->setTextColor(0xFF00FCFF);
		sys::setting::set_music_play_dev(E_AUDIO_TYPE_BT_MUSIC);
		_update_music_info();
		_update_music_progress();
	}
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	_bt_remove_cb();
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
static bool onbtMusicActivityTouchEvent(const MotionEvent &ev) {
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
static void onProgressChanged_musicSeekBar(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged musicSeekBar %d !!!\n", progress);
}

static bool onButtonClick_prevButton(ZKButton *pButton) {
    LOGD(" ButtonClick prevButton !!!\n");
	if (bt::is_calling()) {
		return false;
	}
    bt::music_prev();
    return false;
}

static bool onButtonClick_nextButton(ZKButton *pButton) {
    LOGD(" ButtonClick nextButton !!!\n");
	if (bt::is_calling()) {
		return false;
	}
    bt::music_next();
    return false;
}

static bool onButtonClick_playButton(ZKButton *pButton) {
    LOGD(" ButtonClick playButton !!!\n");
	if (bt::is_calling()) {
		return false;
	}

//    // 确保音源是蓝牙音乐
//    if (audio::get_audio_type() != E_AUDIO_TYPE_BT_MUSIC) {
//        LOGD("[btMusic] Switch audio to BT_MUSIC before play");
//        audio::change_audio_type(E_AUDIO_TYPE_BT_MUSIC);
//    }

    bt::music_is_playing() ? bt::music_pause() : bt::music_play();


	float current_vol = audio::get_system_vol();

	// 找到当前音量对应的步进索引 (0, 0.1, 0.2, ..., 1.6)
	int current_step = (int)((current_vol / 0.1f) + 0.5f);  // 四舍五入到最近的步进
	current_step = (current_step < 0) ? 0 : (current_step > 16) ? 16 : current_step;

	// 计算临时音量 (增加或减少一个步进)
	int temp_step = current_step + 1;
	if (temp_step > 16) {
		temp_step = current_step - 1;
		if (temp_step < 0) {
			temp_step = 0;
		}
	}
	float temp_vol = temp_step * 0.1f;  // 确保按 0.1 的步进


    //	audio::set_system_vol(temp_vol, true);   // 先调整
	set_navibar_PlayVolSeekBar(temp_step);
    //	audio::set_system_vol(current_vol, true); // 恢复原音量
	set_navibar_PlayVolSeekBar(current_step);
    return false;
}

static bool onButtonClick_queryMusicButton(ZKButton *pButton) {
    LOGD(" ButtonClick queryMusicButton !!!\n");
    EASYUICONTEXT->closeActivity("btContactsActivity");
    EASYUICONTEXT->closeActivity("btDialActivity");
    EASYUICONTEXT->closeActivity("btRecordsActivity");
    return false;
}

static bool onButtonClick_phoneButton(ZKButton *pButton) {
    LOGD(" ButtonClick phoneButton !!!\n");
    if (lk::is_connected()) {
    	mbtTipsWindowPtr->showWnd();
    	return false;
    }
    if (bt::is_calling()) {
    	EASYUICONTEXT->openActivity("callingActivity");
        EASYUICONTEXT->closeActivity("btDialActivity");
    } else {
        EASYUICONTEXT->openActivity("btDialActivity");
    }
    EASYUICONTEXT->closeActivity("btContactsActivity");
    EASYUICONTEXT->closeActivity("btRecordsActivity");
    EASYUICONTEXT->closeActivity("btMusicActivity");
    return false;
}

static bool onButtonClick_btrecordButton(ZKButton *pButton) {
    LOGD(" ButtonClick btrecordButton !!!\n");
    if (lk::is_connected()) {
    	mbtTipsWindowPtr->showWnd();
    	return false;
    }
    EASYUICONTEXT->closeActivity("btDialActivity");
    EASYUICONTEXT->openActivity("btRecordsActivity");
    EASYUICONTEXT->closeActivity("btContactsActivity");
    EASYUICONTEXT->closeActivity("btMusicActivity");
    return false;
}

static bool onButtonClick_btcontactsButton(ZKButton *pButton) {
    LOGD(" ButtonClick btcontactsButton !!!\n");
    if (lk::is_connected()) {
    	mbtTipsWindowPtr->showWnd();
    	return false;
    }
    EASYUICONTEXT->closeActivity("btDialActivity");
    EASYUICONTEXT->openActivity("btContactsActivity");
    EASYUICONTEXT->closeActivity("btRecordsActivity");
    EASYUICONTEXT->closeActivity("btMusicActivity");
    return false;
}

static bool onButtonClick_btsettingButton(ZKButton *pButton) {
    LOGD(" ButtonClick btsettingButton !!!\n");
    EASYUICONTEXT->openActivity("btsettingActivity");
    EASYUICONTEXT->closeActivity("btContactsActivity");
    EASYUICONTEXT->closeActivity("btDialActivity");
    EASYUICONTEXT->closeActivity("btRecordsActivity");
    EASYUICONTEXT->closeActivity("btMusicActivity");
    return false;
}
