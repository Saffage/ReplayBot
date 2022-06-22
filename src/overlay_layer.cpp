#include "overlay_layer.hpp"
#include "replay_system.hpp"
#include <nfd.h>
#include <sstream>
#include "recorder_layer.hpp"
#include "nodes.hpp"
#include "version.h"
#include <filesystem>

bool OverlayLayer::init() {
    if (!initWithColor({ 0, 0, 0, 105 })) return false;

    setZOrder(20);

    auto win_size = CCDirector::sharedDirector()->getWinSize();
    auto& rs = ReplaySystem::get();

    rs.load(); // load config every time you open this menu
        
    auto menu = CCMenu::create();
    menu->setPosition({0, win_size.height});
    addChild(menu);

    this->registerWithTouchDispatcher();
    CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

    auto sprite = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    sprite->setScale(0.75f);
    
    auto btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::close_btn_callback));
    btn->setPosition({18, -18});
    menu->addChild(btn);

    sprite = CCSprite::create("GJ_button_01.png");
    sprite->setScale(0.72f);

    auto* const check_off_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    auto* const check_on_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");

    m_record_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OverlayLayer::on_record));
    m_record_toggle->setPosition({35, -50});
    m_record_toggle->toggle(rs.is_recording());
    menu->addChild(m_record_toggle);

    auto label = CCLabelBMFont::create("Record", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.8f);
    label->setPosition({55, win_size.height - 50});
    addChild(label);

    m_play_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OverlayLayer::on_play));
    m_play_toggle->setPosition({35, -85});
    m_play_toggle->toggle(rs.is_playing());
    menu->addChild(m_play_toggle);

    label = CCLabelBMFont::create("Play", "bigFont.fnt");
    label->setAnchorPoint({0, 0.5});
    label->setScale(0.8f);
    label->setPosition({55, win_size.height - 85});
    addChild(label);

    sprite = CCSprite::create("GJ_button_02.png");
    sprite->setScale(0.72f);

    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_save));
    btn->setPosition({win_size.width - 35, -50});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Save", "bigFont.fnt");
    label->setAnchorPoint({1, 0.5});
    label->setScale(0.8f);
    label->setPosition({win_size.width - 55, win_size.height - 50});
    addChild(label);

    btn = gd::CCMenuItemSpriteExtra::create(sprite, this, menu_selector(OverlayLayer::on_load));
    btn->setPosition({win_size.width - 35, -85});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Load", "bigFont.fnt");
    label->setAnchorPoint({1, 0.5});
    label->setScale(0.8f);
    label->setPosition({win_size.width - 55, win_size.height - 85});
    addChild(label);

    auto* const options_sprite = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    options_sprite->setScale(0.67f);

    btn = gd::CCMenuItemSpriteExtra::create(options_sprite, this, menu_selector(OverlayLayer::on_options));
    btn->setPosition({win_size.width - 35, -120});
    menu->addChild(btn);

    label = CCLabelBMFont::create("Settings", "bigFont.fnt");
    label->setAnchorPoint({1, 0.5});
    label->setScale(0.6f);
    label->setPosition({win_size.width - 55, win_size.height - 120});
    addChild(label);

    btn = gd::CCMenuItemSpriteExtra::create(CCSprite::create("GJ_button_01.png"), this, menu_selector(OverlayLayer::on_recorder));
    {
        auto draw_node = CCDrawNode::create();
        constexpr size_t n_verts = 16;
        constexpr float radius = 13.f;
        CCPoint verts[n_verts];
        for (size_t i = 0; i < n_verts; ++i) {
            verts[i] = CCPoint::forAngle(static_cast<float>(i) / n_verts * 6.2831f) * radius;
        }
        draw_node->drawPolygon(verts, n_verts, {1.f, 0.f, 0.f, 1.f}, 1.f, {0.f, 0.f, 0.f, 1.f});
        btn->getNormalImage()->addChild(draw_node);
        draw_node->setPosition(btn->getNormalImage()->getContentSize() / 2.f);
    }
    btn->getNormalImage()->setScale(0.775f);
    btn->setPosition({win_size.width - 35, -155});
    menu->addChild(btn);

    addChild(NodeFactory<CCLabelBMFont>::start("Internal Renderer", "bigFont.fnt")
        .setAnchorPoint(ccp(1, 0.5))
        .setScale(0.6f)
        .setPosition(win_size - ccp(55, 155))
    );

    m_replay_info = CCLabelBMFont::create("", "chatFont.fnt");
    m_replay_info->setAnchorPoint({0, 1});
    m_replay_info->setPosition({20, win_size.height - 130});
    update_info_text();
    addChild(m_replay_info);

    addChild(NodeFactory<CCLabelBMFont>::start("Speedhack", "bigFont.fnt")
        .setAnchorPoint(ccp(0, .5f))
        .setPosition(ccp(20, win_size.height - 115))
        .setScale(0.6f)
    );

    {
        auto input = FloatInputNode::create(CCSize(64.f, 30.f));
        input->set_value(rs.speed_hack);
        input->input_node->setMaxLabelScale(0.7f);
        input->input_node->setMaxLabelLength(10);
        input->setPosition(ccp(162, win_size.height - 115));
        input->callback = [&](auto& input) {
            const auto value = input.get_value();
            rs.speed_hack = value ? value.value() : 1.f;
        };
        addChild(input);
    }
    
    addChild(NodeFactory<CCLabelBMFont>::start(REPLAYBOT_VERSION, "chatFont.fnt")
        .setAnchorPoint(ccp(1, 0))
        .setScale(0.6f)
        .setPosition(ccp(win_size.width - 5, 5))
        .setOpacity(100u)
    );

    setKeypadEnabled(true);
    setTouchEnabled(true);

    return true;
}

void OverlayLayer::update_info_text() {
    auto& rs = ReplaySystem::get();
    auto& replay = rs.get_replay();
    std::stringstream stream;
    stream << "Current Replay:\nFPS: " << replay.get_fps();
    stream << "\nActions: " << replay.get_actions().size();
    stream << "\nMode: " << (replay.get_type() == ReplayType::XPOS ? "X Pos" : "Frame");
    if (replay.get_type() == ReplayType::XPOS) {
        stream << "\n\nX Pos: " << std::setprecision(10) << std::fixed << 
        float(gd::GameManager::sharedState()->getPlayLayer()->m_player1->m_position.x);
    } else if (replay.get_type() == ReplayType::FRAME) {
        stream << "\n\nFrame: " << unsigned(rs.get_frame());
    }
    m_replay_info->setString(stream.str().c_str());
}

void OverlayLayer::FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2) {
    if (!btn2) {
        if (alert->getTag() == 44) {
            CCApplication::sharedApplication()->openURL("https://www.gyan.dev/ffmpeg/builds/");
        } else {
            if (alert->getTag() == 1 || alert->getTag() == 2) {
                auto& rs = ReplaySystem::get();
                rs.toggle_recording();
                update_info_text();
            }
            if (alert->getTag() == 3) {
                _handle_load_replay();
            }
        }
    }
    if (btn2) {
        if (alert->getTag() == 1 || alert->getTag() == 2) {
            m_record_toggle->toggle(false);
            update_info_text();
        }
    }
}

void OverlayLayer::on_options(CCObject*) {
    OptionsLayer::create(this)->show();
}

void OverlayLayer::on_record(CCObject*) {
    auto& rs = ReplaySystem::get();
    auto& replay = rs.get_replay();
    if (!rs.is_recording()) {
        if (rs.get_replay().get_actions().empty()) {
            rs.toggle_recording();
            update_info_text();
        } else if (rs.get_default_fps() == replay.get_fps()) {
            auto alert = gd::FLAlertLayer::create(this,
                "Warning",
                "Ok",
                "Cancel",
                "This will <cr>overwrite</c> all <cb>actions</c> after your current position."
            );
            alert->setTag(2);
            alert->show();
        } else {
            auto alert = gd::FLAlertLayer::create(this,
            "Warning",
            "Ok",
            "Cancel",
            "This will <cr>overwrite</c> your currently loaded replay.");
            alert->setTag(1);
            alert->show();
        }
    } else {
        rs.toggle_recording();
    }
}

void OverlayLayer::on_play(CCObject*) {
    auto& rs = ReplaySystem::get();
    rs.toggle_playing();
    m_record_toggle->toggle(false);
}

void OverlayLayer::on_save(CCObject*) {
    nfdchar_t* path = nullptr;
    auto result = NFD_SaveDialog("rply", nullptr, &path);
    if (result == NFD_OKAY) {
        ReplaySystem::get().get_replay().save(ReplaySystem::change_file_extension(path, "rply"));
        gd::FLAlertLayer::create(nullptr, "Info", "Ok", nullptr, "Replay saved.")->show();
        free(path);
    }
}

void OverlayLayer::_handle_load_replay() {
    nfdchar_t* path = nullptr;
    auto result = NFD_OpenDialog("rply,replay", nullptr, &path);
    if (result == NFD_OKAY) {
        ReplaySystem::get().get_replay() = Replay::load(path);
        update_info_text();
        gd::FLAlertLayer::create(nullptr, "Info", "Ok", nullptr, "Replay loaded.")->show();
        free(path);
    }
}

void OverlayLayer::on_load(CCObject*) {
    auto& rs = ReplaySystem::get();
    if (rs.get_replay().get_actions().empty()) {
        _handle_load_replay();
    } else {
        auto alert = gd::FLAlertLayer::create(this,
        "Warning",
        "Ok",
        "Cancel",
        "This will <cr>overwrite</c> your currently loaded replay.");
        alert->setTag(3);
        alert->show();
    }
}

void OverlayLayer::keyBackClicked() {
    ReplaySystem::get().save();
    gd::FLAlertLayer::keyBackClicked();
}

void OverlayLayer::on_recorder(CCObject*) {
    static bool has_ffmpeg = false;
    if (!has_ffmpeg) {
        wchar_t buffer[MAX_PATH];
        GetModuleFileNameW(GetModuleHandleA(NULL), buffer, MAX_PATH);
        const auto path = std::filesystem::path(buffer).parent_path() / "ffmpeg.exe";
        if (std::filesystem::exists(path)) {
            has_ffmpeg = true;
            ReplaySystem::get().recorder.m_ffmpeg_path = path.string();
        } else {
            // theres prob a way to do it by not spawning a process but im lazy and hate dealing with winapi
            auto process = subprocess::Popen("where ffmpeg");
            if (process.close()) {
                auto popup = gd::FLAlertLayer::create(this, "Error",
                    "Download", "Cancel",
                    "ffmpeg was not found, recorder will not work without it. "
                    "To install ffmpeg download it and place the ffmpeg.exe (found inside the bin folder in the zip) in the gd folder"
                );
                popup->setTag(44);
                popup->show();
            } else
                has_ffmpeg = true;
            if (!has_ffmpeg)
                return;
        }
    }
    RecorderLayer::create()->show();
}

bool OptionsLayer::init(OverlayLayer* parent) {
    m_parent = parent;

    auto win_size = cocos2d::CCDirector::sharedDirector()->getWinSize();

    if (!initWithColor({0, 0, 0, 105})) return false;
    m_pLayer = CCLayer::create();
    addChild(m_pLayer);

    auto bg = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    const CCSize window_size(300, 250);
    bg->setContentSize(window_size);
    bg->setPosition(win_size / 2);
    bg->setZOrder(-5);
    m_pLayer->addChild(bg);

    const CCPoint top_left = win_size / 2.f - ccp(window_size.width / 2.f, -window_size.height / 2.f);

    registerWithTouchDispatcher();
    CCDirector::sharedDirector()->getTouchDispatcher()->incrementForcePrio(2);

    m_pButtonMenu = CCMenu::create();
    m_pButtonMenu->setPosition(top_left);
    auto* const menu = m_pButtonMenu;
    m_pLayer->addChild(m_pButtonMenu);
    auto* const layer = m_pLayer;

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Record mode", "bigFont.fnt")
        .setScale(0.5f)
        .setPosition(top_left + ccp(10.f, -17.f))
        .setAnchorPoint(ccp(0, 0.5f))
    );

    auto* const check_off_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOff_001.png");
    check_off_sprite->setScale(0.75f);
    auto* const check_on_sprite = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    check_on_sprite->setScale(0.75f);

    m_x_pos_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_x_pos));
    m_x_pos_toggle->setPosition(30, -40);
    menu->addChild(m_x_pos_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("X Pos", "bigFont.fnt")
        .setScale(0.5f)
        .setPosition(top_left + ccp(50, -40.5f))
        .setAnchorPoint(ccp(0, 0.5f))
    );

    m_frame_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_frame));
    m_frame_toggle->setPosition(170, -40);
    menu->addChild(m_frame_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Frame", "bigFont.fnt")
        .setScale(0.5f)
        .setPosition(top_left + ccp(190, -40.5f))
        .setAnchorPoint(ccp(0, 0.5f))
    );

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Options", "bigFont.fnt")
        .setScale(0.5f)
        .setPosition(top_left + ccp(10, -67))
        .setAnchorPoint(ccp(0, 0.5f))
    );

    auto& rs = ReplaySystem::get();

    // TODO: add some info button to nodes.hpp
    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png")).setScale(.5f).finish(),
            this, menu_selector(OptionsLayer::on_real_time_info)
        )
        .setPosition(10, -80)
        .setZOrder(-1)
    );

    m_real_time_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_real_time_toggle));
    m_real_time_toggle->setPosition(30, -90);
    menu->addChild(m_real_time_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Real Time", "bigFont.fnt")
        .setScale(0.4f)
        .setPosition(top_left + ccp(50, -90.5f))
        .setAnchorPoint(ccp(0, .5f))
    );

    m_status_text_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_status_text_toggle));
    m_status_text_toggle->setPosition(30, -125);
    menu->addChild(m_status_text_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Status Text", "bigFont.fnt")
        .setScale(0.4f)
        .setPosition(top_left + ccp(50, -125.5f))
        .setAnchorPoint(ccp(0, .5f))
    );

    m_frame_label_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_frame_label_toggle));
    m_frame_label_toggle->setPosition(30, -160);
    menu->addChild(m_frame_label_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Frame Label", "bigFont.fnt")
        .setScale(0.4f)
        .setPosition(top_left + ccp(50, -160.5f))
        .setAnchorPoint(ccp(0, .5f))
    );

    m_dual_type_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_dual_type_toggle));
    m_dual_type_toggle->setPosition(170, -90);
    menu->addChild(m_dual_type_toggle);
    _update_type_buttons();

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Dual Type", "bigFont.fnt")
        .setScale(0.4f)
        .setPosition(top_left + ccp(190, -90.5f))
        .setAnchorPoint(ccp(0, .5f))
    );

    m_plain_text_toggle = gd::CCMenuItemToggler::create(check_off_sprite, check_on_sprite, this, menu_selector(OptionsLayer::on_plain_text_toggle));
    m_plain_text_toggle->setPosition(170, -125);
    menu->addChild(m_plain_text_toggle);

    layer->addChild(
        NodeFactory<CCLabelBMFont>::start("Plain Text", "bigFont.fnt")
        .setScale(0.4f)
        .setPosition(top_left + ccp(190, -125.5f))
        .setAnchorPoint(ccp(0, .5f))
    );

    rs.real_time_mode ?
        m_real_time_toggle->toggle(true):
        m_real_time_toggle->toggle(false);

    rs.status_text ?
        m_status_text_toggle->toggle(true):
        m_status_text_toggle->toggle(false);

    rs.frame_label ?
        m_frame_label_toggle->toggle(true):
        m_frame_label_toggle->toggle(false);

    rs.dual_type_mode ?
        m_dual_type_toggle->toggle(true):
        m_dual_type_toggle->toggle(false);

    rs.plain_text_macro ?
        m_plain_text_toggle->toggle(true):
        m_plain_text_toggle->toggle(false);

    rs.get_default_type() == ReplayType::XPOS ?
        m_x_pos_toggle->toggle(true):
        m_frame_toggle->toggle(true);

    layer->addChild(NodeFactory<CCLabelBMFont>::start("FPS", "bigFont.fnt")
        .setAnchorPoint(ccp(0, 0.5f))
        .setScale(0.7f)
        .setPosition(top_left + ccp(10, -225))
    );

    m_fps_input = NumberInputNode::create(CCSize(64.f, 30.f));

    m_fps_input->set_value(static_cast<int>(rs.get_default_fps()));
    m_fps_input->input_node->setMaxLabelScale(0.7f);
    m_fps_input->input_node->setMaxLabelLength(10);
    m_fps_input->setPosition(top_left + ccp(100, -225));
    layer->addChild(m_fps_input);

    menu->addChild(
        NodeFactory<gd::CCMenuItemSpriteExtra>::start(
            NodeFactory<CCSprite>::start(CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png")).setScale(.75f).finish(),
            this, menu_selector(OptionsLayer::on_close)
        )
        .setPosition(ccp(0, 0))
    );

    setKeypadEnabled(true);
    setTouchEnabled(true);
    setKeyboardEnabled(true);

    return true;
}

void OptionsLayer::keyBackClicked() {
    update_default_fps();
    setKeyboardEnabled(false);
    removeFromParentAndCleanup(true);
}

void OptionsLayer::on_close(CCObject*) {
    keyBackClicked();
}

void OptionsLayer::on_x_pos(CCObject*) {
    m_x_pos_toggle->toggle(false);
    m_frame_toggle->toggle(false);
    ReplaySystem::get().set_default_type(ReplayType::XPOS);
}

void OptionsLayer::on_frame(CCObject*) {
    m_x_pos_toggle->toggle(false);
    m_frame_toggle->toggle(false);
    ReplaySystem::get().set_default_type(ReplayType::FRAME);
}

void OptionsLayer::on_real_time_toggle(CCObject* toggle_) {
    auto toggle = cast<gd::CCMenuItemToggler*>(toggle_);
    if (toggle != nullptr) {
        ReplaySystem::get().real_time_mode = !toggle->isOn();
    }
}

void OptionsLayer::on_status_text_toggle(CCObject* toggle_) {
    auto toggle = cast<gd::CCMenuItemToggler*>(toggle_);
    if (toggle != nullptr) {
        ReplaySystem::get().status_text = !toggle->isOn();
    }
}

void OptionsLayer::on_frame_label_toggle(CCObject* toggle_) {
    auto toggle = cast<gd::CCMenuItemToggler*>(toggle_);
    if (toggle != nullptr) {
        ReplaySystem::get().frame_label = !toggle->isOn();
    }
}

void OptionsLayer::on_dual_type_toggle(CCObject* toggle_) {
    auto toggle = cast<gd::CCMenuItemToggler*>(toggle_);
    auto& rs = ReplaySystem::get();
    if (toggle != nullptr) {
        if (rs.is_recording()) {
            m_dual_type_toggle->toggle(!rs.dual_type_mode);
            FLAlertLayer::create(nullptr, "Warning", "OK", nullptr,
                "Disable <cr>recording</c> mode if you want to change <cb>this</c> option.")->show();
            return;
        }
        rs.dual_type_mode = !toggle->isOn();
        _update_type_buttons();
    }
}

void OptionsLayer::on_plain_text_toggle(CCObject* toggle_) {
    auto toggle = cast<gd::CCMenuItemToggler*>(toggle_);
    if (toggle != nullptr) {
        ReplaySystem::get().plain_text_macro = !toggle->isOn();
    }
}

void OptionsLayer::update_default_fps() {
    ReplaySystem::get().set_default_fps(static_cast<float>(m_fps_input->get_value()));
}

void OptionsLayer::on_real_time_info(CCObject*) {
    FLAlertLayer::create(nullptr, "Info", "OK", nullptr,
        "Will try to run the game at full speed even if the fps doesn't match with the replay's fps.\n"
        "Only in effect when recording or playing.")->show();
}

void OptionsLayer::FLAlert_Clicked(gd::FLAlertLayer* alert, bool btn2) {
    // do nothing
}

void OptionsLayer::_update_type_buttons() {
    auto& rs = ReplaySystem::get();
    if (!rs.dual_type_mode) {
        m_x_pos_toggle->setEnabled(true);
        m_frame_toggle->setEnabled(true);
        if (rs.get_default_type() == ReplayType::XPOS) {
            m_x_pos_toggle->toggle(true);
            m_frame_toggle->toggle(false);
        } else {
            m_x_pos_toggle->toggle(false);
            m_frame_toggle->toggle(true);
        }
    } else {
        m_x_pos_toggle->setEnabled(false);
        m_frame_toggle->setEnabled(false);
        m_x_pos_toggle->toggle(true);
        m_frame_toggle->toggle(true);
    }
}