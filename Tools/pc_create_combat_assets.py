"""전투 프로토타입 1단계 에셋을 만든다. 여러 번 실행해도 된다.

실행: 에디터의 Output Log에서
    py "C:/UnrealEngine/ProjectC/Tools/pc_create_combat_assets.py"
"""
import os

import unreal

ROOT = "/Game/PC"
INPUT_DIR = f"{ROOT}/Input"
COMBAT_DIR = f"{ROOT}/Combat"
ABILITY_DIR = f"{COMBAT_DIR}/Abilities"
MAP_PATH = f"{ROOT}/Maps/Lvl_PC_CombatTest"
CSV_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Data", "DT_PC_Combatants.csv")

MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
UNARMED_ANIM_BP = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def load_or_create(name, folder, asset_class, factory):
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    asset = asset_tools.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError(f"에셋을 만들지 못했습니다: {path}")
    return asset


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text(f'(TagName="{name}")')
    return tag


def make_key(name):
    key = unreal.Key()
    key.import_text(name)
    return key


def save(asset):
    eal.save_loaded_asset(asset)


# ---------- 입력 ----------
def create_input():
    actions = {}
    for name in ("Attack", "Guard", "Bash", "Dodge"):
        actions[name] = load_or_create(f"IA_PC_{name}", INPUT_DIR, unreal.InputAction, unreal.InputAction_Factory())
        save(actions[name])

    context = load_or_create("IMC_PC_Combat", INPUT_DIR, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    context.unmap_all()
    context.map_key(actions["Attack"], make_key("LeftMouseButton"))
    context.map_key(actions["Guard"], make_key("RightMouseButton"))
    context.map_key(actions["Bash"], make_key("F"))
    context.map_key(actions["Dodge"], make_key("LeftShift"))
    save(context)
    return actions, context


# ---------- 기술 데이터 ----------
def create_ability(name, ability_class, tag, **props):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PCAbilityData)
    data = load_or_create(name, ABILITY_DIR, unreal.PCAbilityData, factory)
    data.set_editor_property("ability_class", ability_class)
    data.set_editor_property("ability_tag", make_tag(tag))
    for key, value in props.items():
        data.set_editor_property(key, value)
    save(data)
    return data


def timing(windup, active, recovery):
    value = unreal.PCActionTiming()
    value.set_editor_property("windup", windup)
    value.set_editor_property("active", active)
    value.set_editor_property("recovery", recovery)
    return value


def shape(hit_range, half_angle):
    value = unreal.PCHitShape()
    value.set_editor_property("range", hit_range)
    value.set_editor_property("half_angle_deg", half_angle)
    return value


def create_abilities():
    light = make_tag("Threat.Light")
    return {
        "Slash": create_ability(
            "DA_PC_Slash", unreal.PCAbility_MeleeAttack, "Ability.Attack.Slash",
            timing=timing(0.25, 0.10, 0.35), stamina_cost=12.0,
            damage=15.0, guard_stamina_damage=15.0, stagger_time=0.0,
            stagger_only_on_windup=False, threat_tag=light, hit_shape=shape(190.0, 55.0)),
        "ShieldBash": create_ability(
            "DA_PC_ShieldBash", unreal.PCAbility_MeleeAttack, "Ability.Attack.ShieldBash",
            timing=timing(0.15, 0.08, 0.45), stamina_cost=20.0,
            damage=3.0, guard_stamina_damage=25.0, stagger_time=1.2,
            stagger_only_on_windup=True, threat_tag=light, hit_shape=shape(140.0, 45.0)),
        "Guard": create_ability(
            "DA_PC_Guard", unreal.PCAbility_Guard, "Ability.Guard",
            stamina_cost=0.0, guard_half_angle_deg=70.0),
        "Dodge": create_ability(
            "DA_PC_Dodge", unreal.PCAbility_Dodge, "Ability.Dodge",
            timing=timing(0.0, 0.20, 0.25), stamina_cost=20.0, dodge_distance=350.0),
        "Claw": create_ability(
            "DA_PC_Claw", unreal.PCAbility_MeleeAttack, "Ability.Attack.Claw",
            timing=timing(0.80, 0.12, 0.90), stamina_cost=0.0,
            damage=20.0, guard_stamina_damage=30.0, stagger_time=0.35,
            stagger_only_on_windup=False, threat_tag=light, hit_shape=shape(220.0, 50.0)),
    }


# ---------- 데이터테이블 ----------
def create_data_table():
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", unreal.PCCombatantRow.static_struct())
    table = load_or_create("DT_PC_Combatants", COMBAT_DIR, unreal.DataTable, factory)
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, CSV_PATH):
        raise RuntimeError(f"CSV를 가져오지 못했습니다: {CSV_PATH}")
    save(table)
    return table


def row_handle(table, row_name):
    handle = unreal.DataTableRowHandle()
    handle.set_editor_property("data_table", table)
    handle.set_editor_property("row_name", row_name)
    return handle


# ---------- 블루프린트 ----------
def create_blueprint(name, parent_class):
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return load_or_create(name, COMBAT_DIR, unreal.Blueprint, factory)


def finish_blueprint(blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    save(blueprint)


def defaults_of(blueprint):
    return unreal.get_default_object(blueprint.generated_class())


def set_mannequin(defaults):
    mesh = defaults.get_editor_property("mesh")
    mesh.set_editor_property("skeletal_mesh_asset", eal.load_asset(MANNY_MESH))
    mesh.set_editor_property("anim_class", eal.load_asset(UNARMED_ANIM_BP).generated_class())


def input_binding(action, tag, hold):
    binding = unreal.PCAbilityInputBinding()
    binding.set_editor_property("input_action", action)
    binding.set_editor_property("ability_tag", make_tag(tag))
    binding.set_editor_property("hold", hold)
    return binding


def create_blueprints(actions, context, abilities, table):
    player = create_blueprint("BP_PC_Player", unreal.PCPlayerCharacter)
    defaults = defaults_of(player)
    set_mannequin(defaults)
    defaults.set_editor_property("combatant_row", row_handle(table, "Player"))
    defaults.set_editor_property("starting_abilities", [abilities["Slash"], abilities["ShieldBash"], abilities["Guard"], abilities["Dodge"]])
    defaults.set_editor_property("mapping_contexts", [
        eal.load_asset("/Game/Input/IMC_Default"),
        eal.load_asset("/Game/Input/IMC_MouseLook"),
        context,
    ])
    defaults.set_editor_property("move_action", eal.load_asset("/Game/Input/Actions/IA_Move"))
    defaults.set_editor_property("look_action", eal.load_asset("/Game/Input/Actions/IA_Look"))
    defaults.set_editor_property("mouse_look_action", eal.load_asset("/Game/Input/Actions/IA_MouseLook"))
    defaults.set_editor_property("ability_bindings", [
        input_binding(actions["Attack"], "Ability.Attack.Slash", False),
        input_binding(actions["Bash"], "Ability.Attack.ShieldBash", False),
        input_binding(actions["Guard"], "Ability.Guard", True),
        input_binding(actions["Dodge"], "Ability.Dodge", False),
    ])
    finish_blueprint(player)

    werewolf = create_blueprint("BP_PC_Werewolf", unreal.PCMonsterBase)
    defaults = defaults_of(werewolf)
    set_mannequin(defaults)
    defaults.set_editor_property("combatant_row", row_handle(table, "Werewolf"))
    defaults.set_editor_property("starting_abilities", [abilities["Claw"]])
    defaults.set_editor_property("auto_attack", True)
    defaults.set_editor_property("auto_attack_tag", make_tag("Ability.Attack.Claw"))
    defaults.set_editor_property("auto_attack_interval", 3.0)
    finish_blueprint(werewolf)

    game_mode = create_blueprint("BP_PC_CombatGameMode", unreal.PCCombatGameMode)
    defaults_of(game_mode).set_editor_property("default_pawn_class", player.generated_class())
    finish_blueprint(game_mode)

    return player, werewolf, game_mode


# ---------- 테스트 레벨 ----------
def create_level(werewolf, game_mode):
    if eal.does_asset_exist(MAP_PATH):
        unreal.log(f"레벨이 이미 있어서 건너뜁니다: {MAP_PATH}")
        return

    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError(f"레벨을 만들지 못했습니다: {MAP_PATH}")

    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, -50.0))
    floor.static_mesh_component.set_static_mesh(eal.load_asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(unreal.Vector(40.0, 40.0, 1.0))
    floor.set_actor_label("Floor")

    actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 600.0), unreal.Rotator(pitch=-50.0, yaw=35.0, roll=0.0))
    sky_light = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0.0, 0.0, 600.0))
    sky_light.light_component.set_editor_property("real_time_capture", True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))

    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-400.0, 0.0, 100.0))

    wolf = actors.spawn_actor_from_class(werewolf.generated_class(), unreal.Vector(300.0, 0.0, 130.0), unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0))
    wolf.set_actor_scale3d(unreal.Vector(1.3, 1.3, 1.3))
    wolf.set_actor_label("Werewolf")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", game_mode.generated_class())
    level_editor.save_current_level()


def main():
    actions, context = create_input()
    abilities = create_abilities()
    table = create_data_table()
    _, werewolf, game_mode = create_blueprints(actions, context, abilities, table)
    create_level(werewolf, game_mode)
    unreal.log("전투 프로토타입 에셋 생성 완료. /Game/PC/Maps/Lvl_PC_CombatTest 를 열고 Play 하세요.")


main()
