#include "Plugin.h"
#include "mc/nbt/CompoundTag.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandParameterData.h"
#include "mc/server/commands/CommandPosition.h"
#include "mc/server/commands/CommandRawText.h"
#include "mc/server/commands/CommandRegistry.h"
#include "mc/server/commands/CommandSelectorResults.h"
#include "mc/world/level/Command.h"
#include <direct.h>
#include <ll/api/event/EventBus.h>
#include <ll/api/memory/Hook.h>
#include <ll/api/plugin/NativePlugin.h>
#include <ll\api\event\entity\ActorHurtEvent.h>
#include <ll\api\service/Bedrock.h>
#include <mc/world/level/block/actor/BlockActor.h>
#include "vector"

using namespace std;

namespace plugin {

static map<string, CompoundTag> blockEntityCache;

void createFolder(const ll::Logger& logger, const std::string& folderName) {
    int result = _mkdir(folderName.c_str());
    if (result != 0) {
        logger.error("Failed to create folder.");
    } else {
        logger.info("Folder " + string(folderName) + " created successfully.");
    }
}

vector<string> split(string s, string del) {
    vector<string> re;
    // Use find function to find 1st position of delimiter.
    int end = s.find(del);
    while (end != -1) { // Loop until no delimiter is left in the string.
        re.push_back(s.substr(0, end));
        s.erase(s.begin(), s.begin() + end + 1);
        end = s.find(del);
    }
    re.push_back(s.substr(0, end));
    return re;
}

class ExtCommand : public Command {

public:
    void execute(class CommandOrigin const&, class CommandOutput&) const override {
        auto nbt = CompoundTag();
        for (auto& [k, v] : blockEntityCache) {
            const string& name = k;
            nbt.putCompound(name, v);
        }
        string binary_nbt = nbt.toBinaryNbt(false);
        auto   out        = ofstream("data/blockentity.nbt", ofstream::out | ofstream::binary | ofstream::trunc);
        out << binary_nbt;
        out.close();
    }

    static void setup(CommandRegistry& registry) {
        registry.registerCommand("ext", "excute ext command", CommandPermissionLevel::GameDirectors);
        registry.registerOverload<ExtCommand>("ext");
    }
};

LL_AUTO_TYPED_INSTANCE_HOOK(
    BlockActorHook,
    ll::memory::HookPriority::Normal,
    BlockActor,
    "?load@BlockActor@@UEAAXAEAVLevel@@AEBVCompoundTag@@AEAVDataLoadHelper@@@Z", // BlockActor#load
    void,
    class Level&          level,
    CompoundTag const&    tag,
    class DataLoadHelper& dataLoadHelper
) {
    origin(level, tag, dataLoadHelper);
    const string& name     = tag.getString("id");
    blockEntityCache[name] = tag;
    const ll::Logger logger("hook");
    logger.info("export blockentity " + name);
}

Plugin::Plugin(ll::plugin::NativePlugin& self) : mSelf(self) {
    mSelf.getLogger().info("loading BdsDebugUtil...");
    const ll::Logger& logger = mSelf.getLogger();
    createFolder(logger, "data");
    using namespace ll::event;
    EventBus::getInstance().emplaceListener<ActorHurtEvent>([&logger](ActorHurtEvent& e) {
        if (e.source().getEntityType() == ActorType::Player) {
            const string& view = e.self().saveToNbt()->toSnbt();
            logger.info(view);
            const auto& names = split(e.self().getTypeName(),":");
            const string& path = "data/" + names[1] + ".txt";
            auto          out  = ofstream(path, ofstream::out | ofstream::trunc);
            out << view;
            out.close();
        }
    });
}

bool Plugin::enable() {
    mSelf.getLogger().info("enabling BdsDebugUtil...");
    CommandRegistry& command_registry = ll::service::getCommandRegistry().get();
    ExtCommand::setup(command_registry);
    return true;
}

bool Plugin::disable() {
    mSelf.getLogger().info("disabling BdsDebugUtil...");
    return true;
}

} // namespace plugin
