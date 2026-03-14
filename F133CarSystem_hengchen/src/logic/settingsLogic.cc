#pragma once
#include "uart/ProtocolSender.h"
#include "system/setting.h"
#include "os/SystemProperties.h"
#include "manager/LanguageManager.h"
#include "utils/TimeHelper.h"
#include "system/usb_monitor.h"
#include "bt/context.h"
#include "link/context.h"
#include "utils/BrightnessHelper.h"
#include "fy/date.hpp"
#include "fy/format.hpp"
#include "uart/context.h"
#include "version.h"
#include "misc/storage.h"
#include "utils/BitmapHelper.h"
#include "manager/ConfigManager.h"
#include "mode_observer.h"
#include "media/audio_context.h"
#include <base/time.hpp>
#include "utils/mem_profiler.h"

#define DEFAULT_BT_NAME "BT_BOX"
#define TIMEBIGSIZE 	45 	//时间设置中间字体大小
#define TIMESMALLSIZE 	30	//时间设置两边字体大小

#define UPDATE_DAY 1
//起始年份
#define ORG_YEAR 2020
//年份总个数
//#define YEAR_NUM 67

#define START_YEAR      2010
#define YEAR_NUM        30

static const char* systemTextTab[] = {"language","data","wifi","Debug","version"};

// 修改：将英文放在第一位作为默认语言
static const char* languaueTextTab[] = {
    "English(United States)",      // en_US
    "简体中文",                    // zh_CN
    "Français(France)",           // fr_FRA
    "العربية",                   // ar
    "Czech",                    // cs_CZ
    "Deutsch(Deutschland)",       // de_DE
    "Español(España)",           // es_ES
    "עברית",                     // he_IL
    "Italiano(Italia)",          // it_IT
    "日本語",                     // ja_JP
    "Polski",                    // pl_PL
    "Русский",                   // ru_RU
    "Türkçe",                     // tr_TR
	"Português Europeu"
};
static int _s_days;

static std::string adjustText[] = {
	"luminance","hue","saturation","contrast"
};
static ZKWindow **systemwndtab[] = {&mLanguageWndPtr,&mSetdataWndPtr,NULL,NULL,&mversionWindowPtr};


typedef enum {
	LANGUAGE_E,		// 语言
	DATA_E,			// 时间日期
	WIFI_E,			// wifi
	ADB_E,			// adb
	SYSTEM_E,		// 系统版本
}SYSTEMSHOWTYPE;

// 修改：调整枚举顺序，英文放在第一位
typedef enum {
	US_E,      // 英文
	CH_E,      // 中文
	FR_E,      // 法文
	AR_E,      // 阿拉伯文
	CS_E,      // 捷克文
	DE_F,      // 德文
	ES_E,      // 西班牙文
	HE_E,      // 希伯来文
	IT_E,      // 意大利文
	JA_E,      // 日文
	PL_E,      // 波兰文
	RU_E,      // 俄文
	TR_E,       // 土耳其文
	PT_E
}LANGUAGETYPE;


// 修改：将默认语言类型设置为英文
static LANGUAGETYPE s_language_type = US_E;
static SYSTEMSHOWTYPE s_system_type;
static struct tm ptm;    //日期时间结构体
static bool ThreeFalg = false;

// 修改：更新语言代码映射，对应新的枚举顺序
static void Get_CurrentCode(const std::string Code){
	if(Code == "en_US"){
		s_language_type = US_E;
	}else if(Code == "zh_CN"){
		s_language_type = CH_E;
	}else if(Code == "fr_FRL"){
		s_language_type = FR_E;
	}else if(Code == "arL_AR"){
		s_language_type = AR_E;
	}else if(Code == "cs_CZL"){
		s_language_type = CS_E;
	}else if(Code == "de_FF"){
		s_language_type = DE_F;
	}else if(Code == "es_ESL"){
		s_language_type = ES_E;
	}else if(Code == "he_ILL"){
		s_language_type = HE_E;
	}else if(Code == "it_ITL"){
		s_language_type = IT_E;
	}else if(Code == "ja_JP"){
		s_language_type = JA_E;
	}else if(Code == "pl_PLL"){
		s_language_type = PL_E;
	}else if(Code == "ru_RUL"){
		s_language_type = RU_E;
	}else if(Code == "tr_TRL"){
		s_language_type = TR_E;
	}else if(Code == "pt_BRL"){
		s_language_type = PT_E;
	}else {
		// 修改：未知语言代码时默认使用英文
		s_language_type = US_E;
	}
}

// 修改：添加默认英文处理
static void Update_CurrentCode(LANGUAGETYPE type){
	std::string Code;
	switch(type){
	case CH_E:
		Code = "zh_CN";
		break;
	case US_E:
		Code = "en_US";
		break;
	case FR_E:
		Code = "fr_FRL";
		break;
	case AR_E:
		Code = "arL_AR";
		break;
	case CS_E:
		Code = "cs_CZL";
		break;
	case DE_F:
		Code = "de_FF";
		break;
	case ES_E:
		Code = "es_ESL";
		break;
	case HE_E:
		Code = "he_ILL";
		break;
	case IT_E:
		Code = "it_ITL";
		break;
	case JA_E:
		Code = "ja_JP";
		break;
	case PL_E:
		Code = "pl_PLL";
		break;
	case RU_E:
		Code = "ru_RUL";
		break;
	case TR_E:
		Code = "tr_TRL";
		break;
	case PT_E:
		Code = "pt_BRL";
		break;
	default:
		// 修改：添加默认英文处理
		Code = "en_US";
		break;
	}
	sys::setting::update_localescode(Code.c_str());
}

//static int _get_days() {
//	int year, month;
//	if (myearListViewPtr->getFirstVisibleItemIndex() == (YEAR_NUM - 1)) {
//		year = ORG_YEAR;
//	} else {
//		year = myearListViewPtr->getFirstVisibleItemIndex() + ORG_YEAR + 1;
//	}
//	if (mmonthListViewPtr->getFirstVisibleItemIndex() == 11) {
//		month = 1;
//	} else {
//		month = mmonthListViewPtr->getFirstVisibleItemIndex() + 2;
//	}
//
//    return fy::date::get_days(year, month);
//}

static int _get_days() {
	int year, month;
	if (myearListViewPtr->getFirstVisibleItemIndex() == (YEAR_NUM - 1)) {
		year = START_YEAR;
	} else {
		year = myearListViewPtr->getFirstVisibleItemIndex() + START_YEAR + 1;
	}
	if (mmonthListViewPtr->getFirstVisibleItemIndex() == 11) {
		month = 1;
	} else {
		month = mmonthListViewPtr->getFirstVisibleItemIndex() + 2;
	}

    return fy::date::get_days(year, month);
}

static void _update_time_info() {
	struct tm *t = TimeHelper::getDateTime();
	int year = t->tm_year + 1900;
	int month = t->tm_mon + 1;
	int day = t->tm_mday;
	int hour = t->tm_hour;
	int minute = t->tm_min;

//	LOGD("%d-%d-%d %d:%d\n", year, month, day, hour, minute);

	myearListViewPtr->setSelection((year <= START_YEAR) ? (YEAR_NUM - 1) : (year - START_YEAR - 1));
	mmonthListViewPtr->setSelection((month == 1) ? 11 : (month - 2));
	mdayListViewPtr->setSelection((day == 1) ? (day - 2 + fy::date::get_days(year, month)) : (day - 2));
	mhourListViewPtr->setSelection((hour == 0) ? 23 : (hour - 1));
	mminListViewPtr->setSelection((minute == 0) ? 59 : (minute - 1));

	_s_days = _get_days();
}

static void _update_date() {
	int year, month, day, hour, min;

	if (myearListViewPtr->getFirstVisibleItemIndex() == (YEAR_NUM - 1)) {
		year = START_YEAR;
	} else {
		year = myearListViewPtr->getFirstVisibleItemIndex() + START_YEAR + 1;
	}
	if (mmonthListViewPtr->getFirstVisibleItemIndex() == 11) {
		month = 1;
	} else {
		month = mmonthListViewPtr->getFirstVisibleItemIndex() + 2;
	}
	if (mdayListViewPtr->getFirstVisibleItemIndex() == (fy::date::get_days(year, month) - 1)) {
		day = 1;
	} else {
		day = mdayListViewPtr->getFirstVisibleItemIndex() + 2;
	}
	if (mhourListViewPtr->getFirstVisibleItemIndex() == 23) {
		hour = 0;
	} else {
		hour = mhourListViewPtr->getFirstVisibleItemIndex() + 1;
	}
	if (mminListViewPtr->getFirstVisibleItemIndex() == 59) {
		min = 0;
	} else {
		min = mminListViewPtr->getFirstVisibleItemIndex() + 1;
	}

	struct tm *pt = TimeHelper::getDateTime();

	if ((pt->tm_year != year - 1900) ||
		(pt->tm_mon != month - 1) ||
		(pt->tm_mday != day) ||
		(pt->tm_hour != hour) ||
		(pt->tm_min != min)) {
		struct tm t;

		t.tm_year = year - 1900;
		t.tm_mon = month - 1;
		t.tm_mday = day;
		t.tm_hour = hour;
		t.tm_min = min;
		t.tm_sec = 1;

		sys::setting::set_DateTime(&t);
		_update_time_info();
	}
}

static void _refresh_day_list() {
	int days = _get_days();
	if (_s_days != days) {
		int index = mdayListViewPtr->getFirstVisibleItemIndex();
		if ((index < 0) || (index + 1 > days)) {
			mdayListViewPtr->setSelection(0);
		} else {
			mdayListViewPtr->refreshListView();
		}
	}
}

static void _mcu_cb(MCU_EVENT event, void* data, uint32_t len) {
	std::string txt;
	switch (event) {
	case MCU_VERSION:
		txt = fy::format("MCU: %s", std::string((char*)data, len).c_str());
		mmcuTextViewPtr->setText(txt);
		break;
	}
}

static void systemWnd(bool show) {
	msystemListViewPtr->setVisible(show);
	mresetButtonPtr->setVisible(show);
	mLanguageWndPtr->setVisible(!show);
	mSetdataWndPtr->setVisible(!show);
	mversionWindowPtr->setVisible(!show);
}

static void _event_mode_cb(event_mode_e mode) {
	if (E_EVENT_MODE_GOBACK == mode) {
		if (mLanguageWndPtr->isWndShow() || mSetdataWndPtr->isWndShow() || mversionWindowPtr->isWndShow()) {
//			if (mLanguageWndPtr->isWndShow()) {
//				_update_date();
//			}
			systemWnd(true);
		} else {
			EASYUICONTEXT->goBack();
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
	//{1,  1000},
};

static void _init_setting_info() {
	mTextView4Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView1Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView2Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView5Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView6Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView7Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView8Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mTextView40Ptr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
	mresetConfirmButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);

	mshowButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	mlinkButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	mvoiceButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	mreverseButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	mfactoryButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_DOTS);
	msystemButtonPtr->setLongMode(ZKTextView::E_LONG_MODE_SCROLL_CIRCULAR);
}

/**
 * 当界面构造时触发
 */
static void onUI_init(){
	MEM_LIFECYCLE("settings", "init");
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	Get_CurrentCode(LANGUAGEMANAGER->getCurrentCode());

	// 修改：如果语言管理器返回的是空字符串或未知代码，确保默认使用英文
	if (LANGUAGEMANAGER->getCurrentCode().empty()) {
		s_language_type = US_E;
		Update_CurrentCode(US_E);
	}

	mhourButton12Ptr->setSelected(!sys::setting::is_time_format_24h());
	mhourButton24Ptr->setSelected(sys::setting::is_time_format_24h());
	mode::add_event_mode_cb(_event_mode_cb);
	uart::add_mcubt_cb(_mcu_cb);
	_init_setting_info();
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
	MEM_LIFECYCLE("settings", "show");
	mode::set_switch_mode(E_SWITCH_MODE_GOBACK);
	systemWnd(true);
}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {
	MEM_LIFECYCLE("settings", "hide");

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	MEM_LIFECYCLE("settings", "quit");
	mode::remove_event_mode_cb(_event_mode_cb);
	uart::remove_mcubt_cb(_mcu_cb);
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

static void sonCheck_dateListIsMove(const MotionEvent &ev){
	switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			if(mSetdataWndPtr->isVisible()) {
				ThreeFalg = false;
			}
			break;
		default:
			break;
	}
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
static bool onsettingsActivityTouchEvent(const MotionEvent &ev) {
    switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			sonCheck_dateListIsMove(ev);
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
			break;
		default:
			break;
	}
	return false;

}
static int getListItemCount_LanguageListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_LanguageListView !\n");
	return TABLESIZE(languaueTextTab);
}

static void obtainListItemData_LanguageListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ LanguageListView  !!!\n");
	ZKListView::ZKListSubItem* languagePicSub = pListItem->findSubItemByID(ID_SETTINGS_languagePicSub);
	ZKListView::ZKListSubItem* languageTextSub = pListItem->findSubItemByID(ID_SETTINGS_languageTextSub);
//	if(AR_E == index){
//		languageTextSub->setFontFamily("zar1ZHarmonyOS_Sans_Naskh_Arabic_Regular");
//		languageTextSub->setText(languaueTextTab[index]);
//	}
//	else{
//		languageTextSub->setFontFamily("sans");
//		languageTextSub->setText(languaueTextTab[index]);
//	}
	languageTextSub->setText(languaueTextTab[index]);
	languagePicSub->setSelected(s_language_type == index);
	languageTextSub->setSelected(s_language_type == index);
}

static void onListItemClick_LanguageListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ LanguageListView  !!!\n");
	s_language_type = (LANGUAGETYPE)index;
	Update_CurrentCode(s_language_type);
}

static int getListItemCount_hourListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_hourListView !\n");
	return 24;
}

static void obtainListItemData_hourListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ hourListView  !!!\n");
	char buf [10]={0};
	snprintf(buf,sizeof(buf),"%02d",index);
	pListItem->setText(buf);
	pListItem->setTextColor(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount() ? 0xFFFFFF : 0x9F9F9F);
//	pListItem->setTextSize(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? TIMEBIGSIZE : TIMESMALLSIZE);
	//保存选中项日期
	if(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount()) {
		ptm.tm_hour = index;
		_update_date();
	}
}

static void onListItemClick_hourListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ hourListView  !!!\n");
	pListView->setSelection(((0 == index) ? pListView->getListItemCount() : index)-1);
}

static int getListItemCount_minListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_minListView !\n");
	return 60;
}

static void obtainListItemData_minListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ minListView  !!!\n");
	char minBuf[10] = {0};
	sprintf(minBuf,"%02d",index);    //设置显示内容
	pListItem->setText(minBuf);
	pListItem->setTextColor(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount() ? 0xFFFFFF : 0x9F9F9F);
//	pListItem->setTextSize(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? TIMEBIGSIZE : TIMESMALLSIZE);
	//保存选中项日期
	if(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount()) {
		ptm.tm_min = index;
		_update_date();
	}
}

static void onListItemClick_minListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ minListView  !!!\n");
	pListView->setSelection(((0 == index) ? pListView->getListItemCount() : index)-1);
}

static int getListItemCount_yearListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_yearListView !\n");
	return YEAR_NUM;
}

static void obtainListItemData_yearListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ yearListView  !!!\n");
	pListItem->setText(index + START_YEAR);
	pListItem->setTextColor(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount() ? 0xFFFFFF : 0x9F9F9F);
//	pListItem->setTextSize(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? TIMEBIGSIZE : TIMESMALLSIZE);
	if(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount()) {
		ptm.tm_year = atoi(pListItem->getText().c_str()) - 1900;
		_update_date();
	}
	_refresh_day_list();
}

static void onListItemClick_yearListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ yearListView  !!!\n");
	pListView->setSelection(((0 == index)? pListView->getListItemCount() : index)-1);
}

static int getListItemCount_monthListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_monthListView !\n");
	return 12;
}

static void obtainListItemData_monthListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ monthListView  !!!\n");
	char buf[10] = {0};
	sprintf(buf, "%02d", index+1);
	pListItem->setText(buf);
	pListItem->setTextColor(index == (pListView->getFirstVisibleItemIndex()+1) % pListView->getListItemCount() ? 0xFFFFFF : 0x9F9F9F);
//	pListItem->setTextSize(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? TIMEBIGSIZE : TIMESMALLSIZE);
	//保存选中项月份
	if(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount()) {
		ptm.tm_mon = atoi(pListItem->getText().c_str()) - 1;
		_update_date();
	}
	_refresh_day_list();
}

static void onListItemClick_monthListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ monthListView  !!!\n");
	pListView->setSelection(((0 == index)? pListView->getListItemCount() : index)-1);
}

static int getListItemCount_dayListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_dayListView !\n");
//	return fy::date::get_days(ptm.tm_year, ptm.tm_mon + 1);
	_s_days = _get_days();
	return _s_days;
}



static void obtainListItemData_dayListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ dayListView  !!!\n");
	char buf[32] = {0};
	sprintf(buf,"%02d",index+1);
	pListItem->setText(buf);
//	pListItem->setTextSize(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? TIMEBIGSIZE : TIMESMALLSIZE);
	pListItem->setTextColor(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount() ? 0xFFFFFF : 0x9F9F9F);
	//保存选中项日期
	if(index == (pListView->getFirstVisibleItemIndex()+1)%pListView->getListItemCount()) {
		ptm.tm_mday = index+1;
		_update_date();
	}
}

static void onListItemClick_dayListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ dayListView  !!!\n");
	pListView->setSelection(((0 == index)? pListView->getListItemCount() : index)-1);
}

static int getListItemCount_systemListView(const ZKListView *pListView) {
    //LOGD("getListItemCount_systemListView !\n");
	return TABLESIZE(systemTextTab);
}

static void obtainListItemData_systemListView(ZKListView *pListView,ZKListView::ZKListItem *pListItem, int index) {
    //LOGD(" obtainListItemData_ systemListView  !!!\n");
	ZKListView::ZKListSubItem* systemSubText = pListItem->findSubItemByID(ID_SETTINGS_systemSubText);
	ZKListView::ZKListSubItem* SubItemSysMore = pListItem->findSubItemByID(ID_SETTINGS_SubItemSysMore);
	ZKListView::ZKListSubItem* SubItemAdb = pListItem->findSubItemByID(ID_SETTINGS_SubItemAdb);
	SubItemSysMore->setVisible(index != 3);
	SubItemAdb->setVisible(index == 3);
	SubItemAdb->setSelected(sys::setting::is_usb_adb_enabled());
	systemSubText->setTextTr(systemTextTab[index]);
}

static void onListItemClick_systemListView(ZKListView *pListView, int index, int id) {
    //LOGD(" onListItemClick_ systemListView  !!!\n");
	s_system_type = (SYSTEMSHOWTYPE)index;
	std::string txt;
	std::string apptxt;
	switch (s_system_type) {
	case LANGUAGE_E:		// 语言
		msystemListViewPtr->setVisible(false);
		mresetButtonPtr->setVisible(false);
		(*systemwndtab[LANGUAGE_E])->setVisible(true);
		break;
	case DATA_E:			// 时间日期
//		InitTimeList();
		_update_time_info();
		msystemListViewPtr->setVisible(false);
		mresetButtonPtr->setVisible(false);
		(*systemwndtab[DATA_E])->setVisible(true);
		break;
	case WIFI_E:			// wifi
		EASYUICONTEXT->openActivity("wifisettingActivity");
		break;
	case ADB_E:				// adb
		sys::setting::enable_usb_adb(!sys::setting::is_usb_adb_enabled());
		break;
	case SYSTEM_E:			// version
		char version[32];
		auto dt = base::buildDateTime();
		uart::query_mcubt_version();
		SystemProperties::getString("ro.build.date", version, "");
		txt = fy::format("OS: %s", version);
		apptxt = fy::format("APP: V%d.%d.%d.%04d%02d%02d",APP_MAJOR, APP_MINOR, APP_PATCH,dt.year,dt.month,dt.day);
		mosTextViewPtr->setText(txt);
		mappTextViewPtr->setText(apptxt);

		msystemListViewPtr->setVisible(false);
		mresetButtonPtr->setVisible(false);
		(*systemwndtab[SYSTEM_E])->setVisible(true);
		break;
	}
}

static bool onButtonClick_resetButton(ZKButton *pButton) {
    LOGD(" ButtonClick resetButton !!!\n");
    mresetWindowPtr->showWnd();
    return false;
}

static bool onButtonClick_resetConfirmButton(ZKButton *pButton) {
    LOGD(" ButtonClick resetConfirmButton !!!\n");
    audio::set_system_vol(0);
    DELAY(50);
    bt::disconnect();
    bt::clear_match_device();
    bt::modify_name(DEFAULT_BT_NAME);
    bt::set_auto_answer(false);
    bt::set_auto_connect(false);
    storage::clear();
    system("rm -rf /data/*");
    usleep(1000);
    system("reboot");
    return false;
}

static bool onButtonClick_resetCancelButton(ZKButton *pButton) {
    LOGD(" ButtonClick resetCancelButton !!!\n");
    mresetWindowPtr->hideWnd();
    return false;
}

static bool onButtonClick_hourButton12(ZKButton *pButton) {
    LOGD(" ButtonClick hourButton12 !!!\n");
    sys::setting::set_time_format_24h(false);
    mhourButton24Ptr->setSelected(false);
    pButton->setSelected(!sys::setting::is_time_format_24h());
    return false;
}

static bool onButtonClick_hourButton24(ZKButton *pButton) {
    LOGD(" ButtonClick hourButton24 !!!\n");
    sys::setting::set_time_format_24h(true);
    mhourButton12Ptr->setSelected(false);
    pButton->setSelected(sys::setting::is_time_format_24h());
    return false;
}

static bool onButtonClick_showButton(ZKButton *pButton) {
    LOGD(" ButtonClick showButton !!!\n");
    EASYUICONTEXT->openActivity("setshowActivity");
    EASYUICONTEXT->closeActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setvoiceActivity");
    EASYUICONTEXT->closeActivity("setreverseActivity");
    EASYUICONTEXT->closeActivity("setfactoryActivity");
    EASYUICONTEXT->closeActivity("settingActivity");
    return false;
}

static bool onButtonClick_linkButton(ZKButton *pButton) {
    LOGD(" ButtonClick linkButton !!!\n");
    EASYUICONTEXT->closeActivity("setshowActivity");
    EASYUICONTEXT->openActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setvoiceActivity");
    EASYUICONTEXT->closeActivity("setreverseActivity");
    EASYUICONTEXT->closeActivity("setfactoryActivity");
    EASYUICONTEXT->closeActivity("settingActivity");
    return false;
}

static bool onButtonClick_voiceButton(ZKButton *pButton) {
    LOGD(" ButtonClick voiceButton !!!\n");
    EASYUICONTEXT->openActivity("setvoiceActivity");
    EASYUICONTEXT->closeActivity("setshowActivity");
    EASYUICONTEXT->closeActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setreverseActivity");
    EASYUICONTEXT->closeActivity("setfactoryActivity");
    EASYUICONTEXT->closeActivity("settingActivity");
    return false;
}

static bool onButtonClick_reverseButton(ZKButton *pButton) {
    LOGD(" ButtonClick reverseButton !!!\n");
    EASYUICONTEXT->closeActivity("setshowActivity");
    EASYUICONTEXT->closeActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setvoiceActivity");
    EASYUICONTEXT->openActivity("setreverseActivity");
    EASYUICONTEXT->closeActivity("setfactoryActivity");
    EASYUICONTEXT->closeActivity("settingActivity");
    return false;
}

static bool onButtonClick_factoryButton(ZKButton *pButton) {
    LOGD(" ButtonClick factoryButton !!!\n");
    EASYUICONTEXT->closeActivity("setshowActivity");
    EASYUICONTEXT->closeActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setvoiceActivity");
    EASYUICONTEXT->closeActivity("setreverseActivity");
    EASYUICONTEXT->openActivity("setfactoryActivity");
    EASYUICONTEXT->closeActivity("settingActivity");
    return false;
}

static bool onButtonClick_systemButton(ZKButton *pButton) {
    LOGD(" ButtonClick systemButton !!!\n");
    systemWnd(true);
    EASYUICONTEXT->closeActivity("setshowActivity");
    EASYUICONTEXT->closeActivity("setlinkActivity");
    EASYUICONTEXT->closeActivity("setvoiceActivity");
    EASYUICONTEXT->closeActivity("setreverseActivity");
    EASYUICONTEXT->closeActivity("setfactoryActivity");
    return false;
}
static bool onButtonClick_Button1(ZKButton *pButton) {
    LOGD(" ButtonClick Button1 !!!\n");
    return false;
}
