"""전투 프로토타입 에셋을 만든다.

/Game/PC/ 아래의 에셋은 저장소에 포함되고 에디터에서 직접 고친다.
이 스크립트는 에셋이 없을 때 처음 만드는 용도라서, 이미 있는 에셋은 건드리지 않는다.
스크립트의 값으로 되돌리고 싶으면 --overwrite 를 붙인다 (행동 트리와 레벨은 덮어쓰지 않는다).

실행: 에디터의 Output Log에서
    py "C:/UnrealEngine/ProjectC/Tools/pc_create_combat_assets.py"
    py "C:/UnrealEngine/ProjectC/Tools/pc_create_combat_assets.py" --overwrite
"""
import os
import sys

import unreal

OVERWRITE = "--overwrite" in sys.argv

ROOT = "/Game/PC"
INPUT_DIR = f"{ROOT}/Input"
COMBAT_DIR = f"{ROOT}/Combat"
ABILITY_DIR = f"{COMBAT_DIR}/Abilities"
WEAPON_DIR = f"{COMBAT_DIR}/Weapons"
MONSTER_DIR = f"{COMBAT_DIR}/Monsters"
AI_DIR = f"{COMBAT_DIR}/AI"
MAP_PATH = f"{ROOT}/Maps/Lvl_PC_CombatTest"
DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Data")

MANNY_MESH = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
UNARMED_ANIM_BP = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary


def load_or_create(name, folder, asset_class, factory):
    """(에셋, 값을 설정해야 하는가)를 돌려준다."""
    path = f"{folder}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path), OVERWRITE
    asset = asset_tools.create_asset(name, folder, asset_class, factory)
    if asset is None:
        raise RuntimeError(f"에셋을 만들지 못했습니다: {path}")
    return asset, True


def make_tag(name):
    tag = unreal.GameplayTag()
    tag.import_text(f'(TagName="{name}")')
    return tag


def make_tag_container(*names):
    container = unreal.GameplayTagContainer()
    inner = ",".join(f'(TagName="{name}")' for name in names)
    container.import_text(f"(GameplayTags=({inner}))")
    return container


def make_key(name):
    key = unreal.Key()
    key.import_text(name)
    return key


def save(asset):
    # 이미 있던 에셋의 값을 고친 경우 변경 표시가 안 붙을 수 있어서, 표시와 관계없이 저장한다
    eal.save_loaded_asset(asset, only_if_is_dirty=False)


# ---------- 입력 ----------
# 입력은 기술이 아니라 슬롯에 묶는다. 슬롯에 어떤 기술이 들어가는지는 무기 에셋이 정한다.
INPUT_KEYS = {
    "Primary": "LeftMouseButton",
    "Secondary": "RightMouseButton",
    "Special": "F",
    "Dodge": "LeftShift",
    "Weapon1": "One",
    "Weapon2": "Two",
    "Weapon3": "Three",
}

# 슬롯 방식으로 바꾸기 전의 입력 에셋. 남아 있으면 지운다.
OBSOLETE_INPUT_ASSETS = ("IA_PC_Attack", "IA_PC_Guard", "IA_PC_Bash")


def create_input():
    actions = {}
    for name in INPUT_KEYS:
        actions[name], configure = load_or_create(f"IA_PC_{name}", INPUT_DIR, unreal.InputAction, unreal.InputAction_Factory())
        if configure:
            save(actions[name])

    context, configure = load_or_create("IMC_PC_Combat", INPUT_DIR, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    if configure:
        context.unmap_all()
        for name, key in INPUT_KEYS.items():
            context.map_key(actions[name], make_key(key))
        save(context)
    return actions, context


def remove_obsolete_input():
    for name in OBSOLETE_INPUT_ASSETS:
        path = f"{INPUT_DIR}/{name}"
        if eal.does_asset_exist(path):
            eal.delete_asset(path)


# ---------- 기술 데이터 ----------
def create_ability(name, ability_class, tag, **props):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PCAbilityData)
    data, configure = load_or_create(name, ABILITY_DIR, unreal.PCAbilityData, factory)
    if configure:
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


def shape(hit_range, half_angle, shape_type=unreal.PCHitShapeType.SECTOR, target_rule=unreal.PCHitTargetRule.FIRST_ONLY):
    value = unreal.PCHitShape()
    value.set_editor_property("shape_type", shape_type)
    value.set_editor_property("target_rule", target_rule)
    value.set_editor_property("range", hit_range)
    value.set_editor_property("half_angle_deg", half_angle)
    return value


HIT = unreal.PCHitReactionWhen.HIT
BLOCKED = unreal.PCHitReactionWhen.BLOCKED


def reaction(when, stagger=0.0, knockback=0.0, required=(), ignored=(), effect=None, duration=0.0, magnitude=1.0):
    """타격 반응 한 줄: 조건(when, required, ignored)이 맞으면 결과(경직, 밀려남, 지속 효과)를 준다."""
    value = unreal.PCHitReaction()
    value.set_editor_property("when", when)
    value.set_editor_property("required_target_tags", make_tag_container(*required))
    value.set_editor_property("ignored_target_tags", make_tag_container(*ignored))
    value.set_editor_property("stagger_time", stagger)
    value.set_editor_property("knockback_distance", knockback)
    value.set_editor_property("effect_class", effect)
    value.set_editor_property("effect_duration", duration)
    value.set_editor_property("effect_magnitude", magnitude)
    return value


def projectile(speed, gravity_scale, max_distance):
    value = unreal.PCProjectileSpec()
    value.set_editor_property("speed", speed)
    value.set_editor_property("gravity_scale", gravity_scale)
    value.set_editor_property("max_distance", max_distance)
    return value


def create_abilities():
    light = make_tag("Threat.Light")
    heavy = make_tag("Threat.Heavy")
    all_in_area = unreal.PCHitTargetRule.ALL_IN_AREA
    return {
        # ----- 검과 방패 -----
        "Slash": create_ability(
            "DA_PC_Slash", unreal.PCAbility_MeleeAttack, "Ability.Attack.Slash",
            timing=timing(0.25, 0.10, 0.35), stamina_cost=12.0,
            damage=15.0, guard_stamina_damage=15.0, threat_tag=light, hit_shape=shape(190.0, 55.0),
            move_speed_scale=0.35, hit_stop_time=0.06,
            hit_reactions=[reaction(HIT, knockback=30.0), reaction(BLOCKED, knockback=10.0)]),
        "ShieldBash": create_ability(
            "DA_PC_ShieldBash", unreal.PCAbility_MeleeAttack, "Ability.Attack.ShieldBash",
            timing=timing(0.15, 0.08, 0.45), stamina_cost=20.0,
            damage=3.0, guard_stamina_damage=25.0, threat_tag=light, hit_shape=shape(140.0, 45.0),
            move_speed_scale=0.2, hit_stop_time=0.08,
            hit_reactions=[
                reaction(HIT, knockback=120.0),
                reaction(BLOCKED, knockback=60.0),
                # 타이밍을 맞춰 상대의 예비 동작을 끊는다
                reaction(HIT, stagger=1.2, required=("State.Action.Windup",)),
            ]),
        "Guard": create_ability(
            "DA_PC_Guard", unreal.PCAbility_Guard, "Ability.Guard",
            stamina_cost=0.0, guard_half_angle_deg=70.0, move_speed_scale=0.5),
        # ----- 양손검 -----
        "Cleave": create_ability(
            "DA_PC_Cleave", unreal.PCAbility_MeleeAttack, "Ability.Attack.Cleave",
            timing=timing(0.45, 0.15, 0.55), stamina_cost=22.0,
            damage=26.0, guard_stamina_damage=30.0, threat_tag=light,
            hit_shape=shape(240.0, 80.0, target_rule=all_in_area),
            move_speed_scale=0.25, hit_stop_time=0.09,
            hit_reactions=[reaction(HIT, knockback=80.0), reaction(BLOCKED, knockback=30.0)]),
        "Overhead": create_ability(
            "DA_PC_Overhead", unreal.PCAbility_MeleeAttack, "Ability.Attack.Overhead",
            timing=timing(0.70, 0.12, 0.70), stamina_cost=30.0,
            damage=40.0, guard_stamina_damage=50.0, threat_tag=heavy, hit_shape=shape(210.0, 30.0),
            move_speed_scale=0.1, hit_stop_time=0.12,
            hit_reactions=[
                # 상대가 무엇을 하고 있든 맞으면 끊긴다
                reaction(HIT, stagger=0.6, knockback=60.0),
                reaction(BLOCKED, knockback=150.0),
            ]),
        # ----- 활 -----
        "Shoot": create_ability(
            "DA_PC_Shoot", unreal.PCAbility_RangedAttack, "Ability.Attack.Shoot",
            timing=timing(0.50, 0.05, 0.35), stamina_cost=15.0,
            damage=18.0, guard_stamina_damage=10.0, threat_tag=light,
            move_speed_scale=0.3, hit_stop_time=0.05,
            projectile=projectile(3500.0, 0.25, 4000.0),
            hit_reactions=[
                # 끊지는 못하지만 잠시 느려지게 한다
                reaction(HIT, knockback=20.0, effect=unreal.PCEffect_Slow, duration=2.0, magnitude=0.6),
            ]),
        # ----- 공통 -----
        "Dodge": create_ability(
            "DA_PC_Dodge", unreal.PCAbility_Dodge, "Ability.Dodge",
            timing=timing(0.0, 0.20, 0.25), stamina_cost=20.0, dodge_distance=350.0),
        # 넣고 드는 시간은 무기 에셋에서 온다. 여기의 시간은 다 든 뒤의 짧은 마무리다.
        "SwapWeapon": create_ability(
            "DA_PC_SwapWeapon", unreal.PCAbility_SwapWeapon, "Ability.SwapWeapon",
            timing=timing(0.0, 0.0, 0.10), stamina_cost=0.0, move_speed_scale=0.5),
        # ----- 늑대인간 -----
        "Claw": create_ability(
            "DA_PC_Claw", unreal.PCAbility_MeleeAttack, "Ability.Attack.Claw",
            timing=timing(0.80, 0.12, 0.90), stamina_cost=0.0,
            damage=20.0, guard_stamina_damage=30.0, threat_tag=light, hit_shape=shape(220.0, 50.0),
            move_speed_scale=0.0, hit_stop_time=0.08,
            hit_reactions=[reaction(HIT, stagger=0.35, knockback=90.0), reaction(BLOCKED, knockback=40.0)]),
        "Lunge": create_ability(
            "DA_PC_Lunge", unreal.PCAbility_LungeAttack, "Ability.Attack.Lunge",
            timing=timing(1.00, 0.30, 1.10), stamina_cost=0.0,
            damage=28.0, guard_stamina_damage=45.0, threat_tag=heavy, hit_shape=shape(170.0, 40.0),
            lunge_distance=450.0,
            move_speed_scale=0.0, hit_stop_time=0.12,
            hit_reactions=[reaction(HIT, stagger=0.5, knockback=220.0), reaction(BLOCKED, knockback=110.0)]),
    }


# ---------- 무기 ----------
CUBE = "/Engine/BasicShapes/Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder"


def ability_slot(input_tag, ability, hold=False):
    value = unreal.PCWeaponAbilitySlot()
    value.set_editor_property("input_tag", make_tag(input_tag))
    value.set_editor_property("ability", ability)
    value.set_editor_property("hold", hold)
    return value


def visual_part(mesh_path, socket, scale, raise_on_guard=False):
    value = unreal.PCWeaponVisualPart()
    value.set_editor_property("mesh", eal.load_asset(mesh_path))
    value.set_editor_property("socket", socket)
    value.set_editor_property("scale", unreal.Vector(*scale))
    value.set_editor_property("raise_on_guard", raise_on_guard)
    return value


def create_weapon(name, tag, display_name, slots, sheathe_time, draw_time, visual_parts, held_move_speed_scale=1.0):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PCWeaponData)
    weapon, configure = load_or_create(name, WEAPON_DIR, unreal.PCWeaponData, factory)
    if configure:
        weapon.set_editor_property("weapon_tag", make_tag(tag))
        weapon.set_editor_property("display_name", display_name)
        weapon.set_editor_property("ability_slots", slots)
        weapon.set_editor_property("sheathe_time", sheathe_time)
        weapon.set_editor_property("draw_time", draw_time)
        weapon.set_editor_property("held_move_speed_scale", held_move_speed_scale)
        weapon.set_editor_property("visual_parts", visual_parts)
        save(weapon)
    return weapon


def create_weapons(abilities):
    """목록의 순서가 선택 번호(1, 2, 3)가 된다."""
    return [
        create_weapon(
            "DA_PC_SwordShield", "Weapon.SwordShield", "검과 방패",
            slots=[
                ability_slot("Input.Primary", abilities["Slash"]),
                ability_slot("Input.Secondary", abilities["Guard"], hold=True),
                ability_slot("Input.Special", abilities["ShieldBash"]),
            ],
            # 방패를 팔에 매고 풀어야 해서 가장 오래 걸린다
            sheathe_time=0.5, draw_time=0.6,
            visual_parts=[
                visual_part(CUBE, "hand_r", (0.05, 0.05, 0.9)),
                visual_part(CYLINDER, "hand_l", (0.6, 0.6, 0.05), raise_on_guard=True),
            ]),
        create_weapon(
            "DA_PC_Greatsword", "Weapon.Greatsword", "양손검",
            slots=[
                ability_slot("Input.Primary", abilities["Cleave"]),
                ability_slot("Input.Secondary", abilities["Overhead"]),
            ],
            sheathe_time=0.4, draw_time=0.5, held_move_speed_scale=0.9,
            visual_parts=[visual_part(CUBE, "hand_r", (0.08, 0.08, 1.5))]),
        create_weapon(
            "DA_PC_Bow", "Weapon.Bow", "활",
            slots=[ability_slot("Input.Primary", abilities["Shoot"])],
            sheathe_time=0.3, draw_time=0.4,
            visual_parts=[visual_part(CUBE, "hand_l", (0.04, 0.04, 1.2))]),
    ]


# ---------- 데이터테이블 ----------
def create_data_table(name, row_struct, csv_name):
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    table, configure = load_or_create(name, COMBAT_DIR, unreal.DataTable, factory)
    if configure:
        csv_path = os.path.join(DATA_DIR, csv_name)
        if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, csv_path):
            raise RuntimeError(f"CSV를 가져오지 못했습니다: {csv_path}")
        save(table)
    return table


def row_handle(table, row_name):
    handle = unreal.DataTableRowHandle()
    handle.set_editor_property("data_table", table)
    handle.set_editor_property("row_name", row_name)
    return handle


# ---------- 행동 트리 ----------
def create_behavior_tree():
    name = "ST_PC_MeleeMonster"
    path = f"{AI_DIR}/{name}"
    if eal.does_asset_exist(path):
        return eal.load_asset(path)
    tree = unreal.PCStateTreeBuilderLibrary.create_melee_monster_state_tree(AI_DIR, name)
    if tree is None:
        raise RuntimeError(f"행동 트리를 만들지 못했습니다: {path}")
    save(tree)
    return tree


# ---------- 몬스터 정의 ----------
def attack_option(tag, min_range, max_range, weight=1.0, cooldown=0.0, rest_after=1.0, min_health=0.0, max_health=1.0):
    option = unreal.PCMonsterAttackOption()
    option.set_editor_property("ability_tag", make_tag(tag))
    option.set_editor_property("min_range", min_range)
    option.set_editor_property("max_range", max_range)
    option.set_editor_property("weight", weight)
    option.set_editor_property("cooldown", cooldown)
    option.set_editor_property("min_health_ratio", min_health)
    option.set_editor_property("max_health_ratio", max_health)
    option.set_editor_property("rest_after", rest_after)
    return option


def create_monsters(abilities, stats_table, rank_table, tree):
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.PCMonsterData)
    werewolf, configure = load_or_create("DA_PC_Werewolf", MONSTER_DIR, unreal.PCMonsterData, factory)
    if configure:
        werewolf.set_editor_property("display_name", "늑대인간")
        werewolf.set_editor_property("rank_row", row_handle(rank_table, "Normal"))
        werewolf.set_editor_property("habitat_tags", make_tag_container("Monster.Habitat.Forest"))
        werewolf.set_editor_property("stats_row", row_handle(stats_table, "Werewolf"))
        werewolf.set_editor_property("abilities", [abilities["Claw"], abilities["Lunge"]])
        werewolf.set_editor_property("attack_options", [
            # 거리는 대상의 몸 표면까지의 거리
            attack_option("Ability.Attack.Claw", 0.0, 180.0, weight=1.0, cooldown=0.0, rest_after=1.2),
            attack_option("Ability.Attack.Lunge", 300.0, 600.0, weight=1.0, cooldown=6.0, rest_after=1.8),
        ])
        werewolf.set_editor_property("behavior_tree", tree)
        werewolf.set_editor_property("detection_range", 1500.0)
        werewolf.set_editor_property("lose_target_range", 2500.0)
        werewolf.set_editor_property("turn_rate", 240.0)
        werewolf.set_editor_property("aim_tolerance_deg", 12.0)
        werewolf.set_editor_property("max_aim_time", 0.8)
        werewolf.set_editor_property("rest_move_speed_scale", 0.35)
        werewolf.set_editor_property("rest_distance_min", 220.0)
        werewolf.set_editor_property("rest_distance_max", 380.0)
        werewolf.set_editor_property("scale", 1.3)
        save(werewolf)
    return {"Werewolf": werewolf}


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


def slot_binding(action, input_tag):
    binding = unreal.PCWeaponSlotInputBinding()
    binding.set_editor_property("input_action", action)
    binding.set_editor_property("input_tag", make_tag(input_tag))
    return binding


def create_blueprints(actions, context, abilities, weapons, stats_table, monsters):
    player, configure = create_blueprint("BP_PC_Player", unreal.PCPlayerCharacter)
    if configure:
        defaults = defaults_of(player)
        set_mannequin(defaults)
        defaults.set_editor_property("combatant_row", row_handle(stats_table, "Player"))
        # 무기와 상관없는 기술만 캐릭터가 직접 갖는다. 나머지는 든 무기가 준다.
        defaults.set_editor_property("starting_abilities", [abilities["Dodge"]])
        defaults.set_editor_property("mapping_contexts", [
            eal.load_asset("/Game/Input/IMC_Default"),
            eal.load_asset("/Game/Input/IMC_MouseLook"),
            context,
        ])
        defaults.set_editor_property("move_action", eal.load_asset("/Game/Input/Actions/IA_Move"))
        defaults.set_editor_property("look_action", eal.load_asset("/Game/Input/Actions/IA_Look"))
        defaults.set_editor_property("mouse_look_action", eal.load_asset("/Game/Input/Actions/IA_MouseLook"))
        defaults.set_editor_property("ability_bindings", [
            input_binding(actions["Dodge"], "Ability.Dodge", False),
        ])
        defaults.set_editor_property("weapon_slot_bindings", [
            slot_binding(actions["Primary"], "Input.Primary"),
            slot_binding(actions["Secondary"], "Input.Secondary"),
            slot_binding(actions["Special"], "Input.Special"),
        ])
        defaults.set_editor_property("weapon_select_actions", [actions["Weapon1"], actions["Weapon2"], actions["Weapon3"]])

        equipment = defaults.get_editor_property("equipment")
        equipment.set_editor_property("loadout", weapons)
        equipment.set_editor_property("starting_index", 0)
        equipment.set_editor_property("swap_ability", abilities["SwapWeapon"])
        equipment.set_editor_property("swap_interrupt_stagger_time", 0.6)
        equipment.set_editor_property("swap_damage_taken_multiplier", 1.0)
        finish_blueprint(player)

    # 몬스터 블루프린트는 겉모습만 갖는다. 수치, 기술, 행동은 몬스터 정의 에셋에서 온다.
    werewolf, configure = create_blueprint("BP_PC_Werewolf", unreal.PCMonsterBase)
    if configure:
        defaults = defaults_of(werewolf)
        set_mannequin(defaults)
        defaults.set_editor_property("monster_data", monsters["Werewolf"])
        defaults.set_editor_property("combatant_row", unreal.DataTableRowHandle())
        defaults.set_editor_property("starting_abilities", [])
        finish_blueprint(werewolf)

    game_mode, configure = create_blueprint("BP_PC_CombatGameMode", unreal.PCCombatGameMode)
    if configure:
        defaults_of(game_mode).set_editor_property("default_pawn_class", player.generated_class())
        finish_blueprint(game_mode)

    return player, werewolf, game_mode


# ---------- 테스트 레벨 ----------
def create_level(werewolf, game_mode):
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if eal.does_asset_exist(MAP_PATH):
        level_editor.load_level(MAP_PATH)
    else:
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

        wolf = actors.spawn_actor_from_class(werewolf.generated_class(), unreal.Vector(600.0, 0.0, 130.0), unreal.Rotator(pitch=0.0, yaw=180.0, roll=0.0))
        wolf.set_actor_label("Werewolf")

        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        world.get_world_settings().set_editor_property("default_game_mode", game_mode.generated_class())

    # 몬스터가 길을 찾을 수 있도록 내비게이션 영역을 둔다 (바닥 전체를 덮는 크기)
    if not any(isinstance(actor, unreal.NavMeshBoundsVolume) for actor in actors.get_all_level_actors()):
        volume = actors.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 200.0))
        volume.set_actor_scale3d(unreal.Vector(42.0, 42.0, 6.0))
        volume.set_actor_label("NavBounds")

    level_editor.save_current_level()


def main():
    actions, context = create_input()
    abilities = create_abilities()
    weapons = create_weapons(abilities)
    stats_table = create_data_table("DT_PC_Combatants", unreal.PCCombatantRow.static_struct(), "DT_PC_Combatants.csv")
    rank_table = create_data_table("DT_PC_MonsterRanks", unreal.PCMonsterRankRow.static_struct(), "DT_PC_MonsterRanks.csv")
    tree = create_behavior_tree()
    monsters = create_monsters(abilities, stats_table, rank_table, tree)
    _, werewolf, game_mode = create_blueprints(actions, context, abilities, weapons, stats_table, monsters)
    remove_obsolete_input()
    create_level(werewolf, game_mode)
    unreal.log("전투 프로토타입 에셋 준비 완료. /Game/PC/Maps/Lvl_PC_CombatTest 를 열고 Play 하세요.")


main()
