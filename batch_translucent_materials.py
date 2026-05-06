r"""
batch_translucent_materials.py

Run from UE5 Output Log Python input bar:
    exec(open(r"C:/Users/Kyron/OneDrive/Documents/Unreal Projects/ProjectSpiritless/batch_translucent_materials.py").read())

Patches every Material in /Game/sA_StylizedForest_Environment to Masked blend
mode and adds a ScalarParameter("Opacity", 1.0) wired to OpacityMask, so the
camera-occlusion C++ system can hide occluding forest geometry at runtime.

Masked is used instead of Translucent because the forest pack uses Nanite meshes
and Nanite does not support Translucent blend mode.

Already-Masked materials are skipped so re-runs are safe.
"""

import unreal

mat_lib   = unreal.MaterialEditingLibrary
asset_lib = unreal.EditorAssetLibrary
reg       = unreal.AssetRegistryHelpers.get_asset_registry()

FOLDER     = "/Game/sA_StylizedForest_Environment"
PARAM_NAME = "Opacity"

filt = unreal.ARFilter(
    class_names     = ["Material"],
    package_paths   = [FOLDER],
    recursive_paths = True,
)
asset_data_list = reg.get_assets(filt)
total           = len(asset_data_list)

updated = 0
skipped = 0
failed  = 0

with unreal.ScopedSlowTask(total, "Patching forest materials…") as task:
    task.make_dialog(True)

    for asset_data in asset_data_list:
        if task.should_cancel():
            unreal.log_warning("[BatchMaterials] Cancelled by user.")
            break

        task.enter_progress_frame(1, str(asset_data.package_name))

        mat = asset_data.get_asset()
        if not isinstance(mat, unreal.Material):
            skipped += 1
            continue

        # Skip already-Masked — already processed
        if mat.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_MASKED:
            skipped += 1
            continue

        try:
            # Add ScalarParameter("Opacity", 1.0) wired directly to OpacityMask.
            # The C++ occlusion system drives this from 1.0 (visible) to 0.0 (hidden).
            opacity_node = mat_lib.create_material_expression(
                mat,
                unreal.MaterialExpressionScalarParameter,
                -400, 400,
            )
            opacity_node.set_editor_property("parameter_name", PARAM_NAME)
            opacity_node.set_editor_property("default_value",  1.0)

            mat_lib.connect_material_property(
                opacity_node, "",
                unreal.MaterialProperty.MP_OPACITY_MASK,
            )

            mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)

            mat_lib.recompile_material(mat)
            asset_lib.save_loaded_asset(mat, False)
            updated += 1

        except Exception as e:
            unreal.log_error(f"[BatchMaterials] Failed on {asset_data.package_name}: {e}")
            failed += 1

unreal.log(
    f"[BatchMaterials] Done — {updated} updated, {skipped} skipped, {failed} failed "
    f"(out of {total} total)."
)
