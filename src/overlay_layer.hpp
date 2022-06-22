#pragma once
#include "includes.h"
#include "nodes.hpp"

class OverlayLayer : public gd::FLAlertLayer, public CCTextFieldDelegate, public gd::FLAlertLayerProtocol {
    CCLabelBMFont* m_replay_info;

    void _handle_load_replay();
public:
    gd::CCMenuItemToggler* m_record_toggle;
    gd::CCMenuItemToggler* m_play_toggle;
    
    static auto create() {
        auto node = new OverlayLayer;
        if (node && node->init()) {
            node->autorelease();
        } else {
            CC_SAFE_DELETE(node);
        }
        return node;
    }

    virtual bool init();

    void update_info_text();

    void open_btn_callback(CCObject*) {
        auto node = create();
        CCDirector::sharedDirector()->getRunningScene()->addChild(node);
    }

    void close_btn_callback(CCObject*) {
        keyBackClicked();
    }

    // maybe do like on_btn_record idk
    void on_record(CCObject*);
    void on_play(CCObject*);
    void on_save(CCObject*);
    void on_load(CCObject*);

    void on_options(CCObject*);
    void on_recorder(CCObject*);

    virtual void keyBackClicked();
    virtual void keyDown(enumKeyCodes key) {
        // keyDown overwrites keyBackClicked, how fun
        if (key == 27) keyBackClicked();
    }
    virtual void FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2);
};

class OptionsLayer : public gd::FLAlertLayer, public gd::FLAlertLayerProtocol {
public:
    NumberInputNode* m_fps_input;
    gd::CCMenuItemToggler* m_x_pos_toggle;
    gd::CCMenuItemToggler* m_frame_toggle;
    gd::CCMenuItemToggler* m_real_time_toggle;
    gd::CCMenuItemToggler* m_status_text_toggle;
    gd::CCMenuItemToggler* m_frame_label_toggle;
    gd::CCMenuItemToggler* m_dual_type_toggle;
    gd::CCMenuItemToggler* m_plain_text_toggle;
    OverlayLayer* m_parent;

    GEN_CREATE(OptionsLayer)

    bool init(OverlayLayer*);
    void on_close(CCObject*);
    void on_x_pos(CCObject*);
    void on_frame(CCObject*);
    void on_real_time_toggle(CCObject*);
    void on_real_time_info(CCObject*);
    void on_status_text_toggle(CCObject*);
    void on_frame_label_toggle(CCObject*);
    void on_dual_type_toggle(CCObject*);
    void on_plain_text_toggle(CCObject*);
    void _update_type_buttons();

    void update_default_fps();

    virtual void keyBackClicked();
    virtual void FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2);
    // TODO: pressing space in the layer just crashes???
    virtual void keyDown(enumKeyCodes key) {
        if (key == 27) keyBackClicked();
    }
};
