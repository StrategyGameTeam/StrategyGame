#include "loading_state.hpp"
#include "test_state.hpp"
#include "packets.hpp"
#include "game_packets.hpp"

Loading_State::Loading_State(State_Stack& arg_state_stack_handle,
                             std::string &ip, int port,
                             std::string &game_id, std::string &nickname) :
	State_Base(arg_state_stack_handle),
    gs(std::make_shared<GameState>()),
    connection(std::make_shared<Connection>(ip, port)),
    game_id(game_id),
    nickname(nickname),
    status_box("Wczytywanie...", 0, 0, 400, 200, 40.f)
{
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();

    connection->registerPacketHandler(ProxyDataPacket::packetId, [&](PacketReader &reader) {
        auto packet = ProxyDataPacket::deserialize(reader);
        std::cout << "Players:" << std::endl;
        for (const auto &item: packet.players){
            std::cout << item << std::endl;
        }
        gs->is_host = packet.is_host;
        gs->players = packet.players;
        gs->nickname = this->nickname;
        gs->game_id = packet.game_id;
        if(gs->is_host){
            auto def_gen_id = state_stack_handle.resourceStore.FindGeneratorIndex("default");
            if (def_gen_id == -1) {
                logger::debug("No world generator found, abroting");
                abort();
            }
            logger::debug("BEFORE WORLDGEN");
            auto& def_gen = state_stack_handle.resourceStore.m_worldgens.at(def_gen_id);
            logger::debug("JUST BEFORE WORLDGEN");
            gs->RunWorldgen(state_stack_handle.moduleLoader, *def_gen, {});
            logger::debug("AFTER WORLDGEN");

            // reveal a starting area
            gs->world.value().at_ref_normalized(HexCoords::from_axial(1, 1)).setFractionVisibility(gs->pretend_fraction, HexData::Visibility::SUPERIOR);
            for(auto c : HexCoords::from_axial(1, 1).neighbours()) {
                gs->world.value().at_ref_normalized(c).setFractionVisibility(gs->pretend_fraction, HexData::Visibility::SUPERIOR);
            }
        }
    });
    connection->registerPacketHandler(WorldUpdatePacket::packetId, [&](PacketReader &reader){
        auto packet = WorldUpdatePacket::deserialize(reader);
        std::cout << "UPDATE WORLD" << std::endl;
        this->gs->world = std::move(packet.world);
    });
    connection->writeToHost(LoginPacket{game_id, nickname});

    adjust_to_window();
}

void Loading_State::handle_events()
{
    connection->handleTasks();
    if(this->gs->world.has_value()){
        connection->clearHandlers();
        state_stack_handle.request_push<Test_State>(connection, gs);
    }
}

void Loading_State::update(double dt)
{
}

void Loading_State::render()
{
    ClearBackground(BLACK);
    status_box.draw();
}

void Loading_State::adjust_to_window() {
    status_box.set_position(GetScreenWidth()/2.f, GetScreenHeight()/2.f);
}
