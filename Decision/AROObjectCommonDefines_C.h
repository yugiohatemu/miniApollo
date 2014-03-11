//
//  AROObjectCommonDefines_C.h
//  apollo-ios-core
//
//  Created by Adam Kinsman on 2013-05-08.
//  Copyright (c) 2013 Arroware Industries Inc. All rights reserved.
//

#ifndef AROObjectCommonDefines_C_h
#define AROObjectCommonDefines_C_h

#if defined(__ANDROID__)
#include <android/log.h>
#endif

#define AOc_CheckIncomingNULL	1
#define AOc_MaxFilenameLength	500
#define AOc_MaxTokenFieldSize	1000

// basic object identifiers
#define AOc_AROObjectTag_header             "ApolloContentObject"
#define AOc_AROObjectTag_init               "init"
#define AOc_AROObjectTag_version            "version"
#define AOc_AROObjectTag_timestamp          "ts"
#define AOc_AROObjectTag_syncId             "sync"
#define AOc_AROObjectTag_hash               "hash"
#define AOc_AROObjectTag_refSyncId          "refs"
#define AOc_AROObjectTag_refHash            "refh"
#define AOc_AROObjectTag_directive          "Directive"
#define AOc_AROObjectTag_type               "type"
#define AOc_AROObjectTag_appDirective       "ApplicationDirectives"
#define AOc_AROObjectTag_consumeBundle      "conBun"
#define AOc_AROObjectTag_consumeAction      "conAct"
#define AOc_AROObjectTag_consumeStart       "conSt"
#define AOc_AROObjectTag_consumeEnd         "conEn"
#define AOc_AROObjectTag_consumeMask        "conMsk"
#define AOc_AROObjectTag_consumeFormat      "conFmt"
#define AOc_AROObjectTag_objectId           "uuid"
//#define AOc_AROObjectTag_label              "lbl"
#define AOc_AROObjectTag_value              "val"
#define AOc_AROObjectTag_localURL           "locURL"
#define AOc_AROObjectTag_richText           "rtxt"
#define AOc_AROObjectTag_richTextColour     "rtxtc"
#define AOc_AROObjectTag_picture            "pic"
#define AOc_AROObjectTag_pictureMessage     "picM"
#define AOc_AROObjectTag_pictureX           "picX"
#define AOc_AROObjectTag_pictureY           "picY"
#define AOc_AROObjectTag_pictureW           "picW"
#define AOc_AROObjectTag_pictureH           "picH"
#define AOc_AROObjectTag_contentId          "cId"
#define AOc_AROObjectTag_conFmtLocal        "conFmtLcl"
#define AOc_AROObjectTag_conFmtRaw          "conFmtRaw"

// identifiers for application types
#define AOc_AROApplication                 "ApolloApp"  // undefined
#define AOc_AROidList                      "ApolloidList"
#define AOc_AROidList_version              "0.0.1"
#define AOc_AROBundleLoader                "ApolloBundleLoader"
#define AOc_AROBundleLoader_version        "0.0.1"
#define AOc_AROConversation                "ApolloConversation"
#define AOc_AROConversation_version        "0.0.1"
#define AOc_AROConversationList            "ApolloConversationList"
#define AOc_AROConversationList_version    "0.0.1"
#define AOc_AROPictureAlbum                "ApolloPictureAlbum"
#define AOc_AROPictureAlbum_version        "0.0.1"
#define AOc_AROPicture              	   "ApolloPicture"
#define AOc_AROPicture_version             "0.0.1"
#define AOc_ARONetworkFeed                 "ApolloNetworkFeed"
#define AOc_ARONetworkFeed_version         "0.0.1"
#define AOc_AROMusicReader                 "ApolloMusicReader"
#define AOc_AROMusicReader_version         "0.0.1"
#define AOc_AROMusicItem                   "ApolloMusicItem"
#define AOc_AROMusicItem_version           "0.0.1"
#define AOc_AROMusicQuery                  "ApolloMusicQuery"
#define AOc_AROMusicQuery_version          "0.0.1"
#define AOc_AROUserRecord		           "ApolloUserRecord"
#define AOc_AROUserRecord_version          "0.0.1"
#define AOc_AROEmail                       "ApolloEmail"
#define AOc_AROEmail_version               "0.0.1"

#define AOc_malloc 	umalloc
#define AOc_strdup 	ustrdup
#define AOc_free 	ufree
#define AOc_realloc urealloc
#if defined(__ANDROID__)
#define AOc_printf(...)  __android_log_print(ANDROID_LOG_DEBUG, "AROLog", __VA_ARGS__);
#else
#define AOc_printf(...) AROLog_Print(logDEBUG,0,"AROLog",__VA_ARGS__);
#endif

/*
extern void *umalloc(int size);
extern void *urealloc(void *ptr, int size);
extern char *ustrdup(char *src);
extern void ufree(void *ptr);
extern int ugetmalloccounter(void);
extern void usetmalloccounter(int val);
*/

#endif
