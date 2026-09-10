#if UNITY_EDITOR
using System;
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEngine;
using Signal47.Core;
using Signal47.Chapter;
using Object = UnityEngine.Object;

namespace Signal47.Editor
{
    /// <summary>
    /// Authored photographic laboratory and the reachable B-12 optical station.
    /// World geometry, material authoring and interaction placement live here;
    /// photographic state and the experiment are owned by ChapterInvestigation.
    /// </summary>
    public static class Chapter09Pass
    {
        const string Folder = "Assets/Signal47/Art/Chapter09/";
        static Transform root;
        static Font font;
        static readonly Dictionary<string, Material> surfaces = new Dictionary<string, Material>();
        static readonly Color PaperInk = new Color(.035f, .045f, .030f);
        static readonly Color LabelInk = new Color(.85f, .84f, .69f);

        static Material Surface(string name, Color color, float roughness = .65f, float metal = 0f, Color emission = default)
        {
            if (surfaces.TryGetValue(name, out var existing) && existing) return existing;
            var path = Folder + name + ".mat";
            var material = AssetDatabase.LoadAssetAtPath<Material>(path);
            if (!material)
            {
                material = new Material(Shader.Find("Universal Render Pipeline/Lit")) { name = name };
                AssetDatabase.CreateAsset(material, path);
            }
            material.SetColor("_BaseColor", color);
            material.SetFloat("_Smoothness", 1 - roughness);
            material.SetFloat("_Metallic", metal);
            if (emission != default)
            {
                material.EnableKeyword("_EMISSION");
                material.SetColor("_EmissionColor", emission);
            }
            EditorUtility.SetDirty(material);
            surfaces[name] = material;
            return material;
        }

        static Material M(string key) => surfaces["CH09_" + key];

        static void Materials()
        {
            surfaces.Clear();
            Surface("CH09_Cream", new Color(.64f, .61f, .49f), .52f, .02f);
            Surface("CH09_Green", new Color(.20f, .27f, .23f), .58f, .05f);
            Surface("CH09_Steel", new Color(.27f, .31f, .30f), .31f, .72f);
            Surface("CH09_Rubber", new Color(.025f, .030f, .026f), .86f);
            Surface("CH09_Paper", new Color(.85f, .82f, .68f), .90f);
            Surface("CH09_InstructionPaper", new Color(.96f, .94f, .82f), .95f);
            Surface("CH09_Amber", new Color(.22f, .10f, .025f), .25f, .1f);
            Surface("CH09_Red", new Color(.30f, .055f, .025f), .48f);
            Surface("CH09_Water", new Color(.025f, .058f, .054f), .16f, .1f);
            Surface("CH09_UpperWall", new Color(.59f, .57f, .46f), .93f);
            Surface("CH09_LowerWall", new Color(.17f, .25f, .22f), .78f);
            Surface("CH09_Trim", new Color(.34f, .39f, .32f), .61f, .1f);
            Surface("CH09_Cork", new Color(.28f, .19f, .10f), .95f);
            Surface("CH09_Concrete", new Color(.31f, .32f, .28f), .96f);
            Surface("CH09_Stripe", new Color(.86f, .87f, .70f), .80f, 0, new Color(.13f, .16f, .10f));
            // This response belongs only to the hidden capture mesh. It keeps
            // the film's retained reference legible after the physical lamp is
            // shielded, without illuminating the plate or the real-world vane.
            Surface("CH09_FilmStripe", new Color(.78f, .84f, .77f), .85f, 0, new Color(.32f, .36f, .34f));
            Surface("CH09_WarmDiffuser", new Color(.98f, .80f, .44f), .7f, 0, new Color(.9f, .48f, .16f) * 1.2f);
            Surface("CH09_RedDiffuser", new Color(.65f, .16f, .065f), .7f, 0, new Color(.45f, .075f, .025f));
            Surface("CH09_LightBox", new Color(.77f, .79f, .61f), .7f, 0, new Color(.51f, .58f, .37f) * .75f);
            var original = AssetDatabase.LoadAssetAtPath<Material>("Assets/Signal47/Art/ThirdParty/PolyHaven/old_linoleum_flooring_01/old_linoleum_flooring_01.mat");
            var floor = AssetDatabase.LoadAssetAtPath<Material>(Folder + "CH09_Linoleum.mat");
            if (!floor)
            {
                floor = original ? new Material(original) : new Material(M("Cream"));
                floor.name = "CH09_Linoleum";
                AssetDatabase.CreateAsset(floor, Folder + "CH09_Linoleum.mat");
            }
            floor.SetTextureScale("_BaseMap", Vector2.one);
            floor.SetTextureScale("_BumpMap", Vector2.one);
            floor.SetTextureScale("_MetallicGlossMap", Vector2.one);
            surfaces["CH09_Linoleum"] = floor;
            EditorUtility.SetDirty(floor);
        }

        static GameObject Box(string name, Vector3 position, Vector3 size, Material material, bool collision = false, Transform parent = null)
        {
            var go = Signal47SceneBuilder.Cube(name, position, size, material, collision);
            go.transform.SetParent(parent ? parent : root, true);
            return go;
        }

        static GameObject LocalBox(Transform parent, string name, Vector3 position, Vector3 size, Material material, bool collision = false)
        {
            var go = Box(name, Vector3.zero, size, material, collision, parent);
            go.transform.localPosition = position;
            go.transform.localRotation = Quaternion.identity;
            return go;
        }

        static GameObject Rod(string name, Vector3 from, Vector3 to, float diameter, Material material, bool collision = false, Transform parent = null)
        {
            var go = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
            go.name = name;
            go.transform.SetParent(parent ? parent : root, true);
            go.transform.position = (from + to) * .5f;
            go.transform.rotation = Quaternion.FromToRotation(Vector3.up, to - from);
            go.transform.localScale = new Vector3(diameter, (to - from).magnitude * .5f, diameter);
            go.GetComponent<Renderer>().sharedMaterial = material;
            if (!collision) Object.DestroyImmediate(go.GetComponent<Collider>());
            return go;
        }

        static GameObject Label(string name, string words, Vector3 position, Vector2 size, Quaternion rotation, Color color, Transform parent = null)
        {
            var go = new GameObject(name);
            go.transform.SetParent(parent ? parent : root, true);
            go.transform.position = position;
            go.transform.rotation = rotation;
            var text = go.AddComponent<TextMesh>();
            text.text = words;
            text.font = font;
            text.fontSize = 64;
            text.characterSize = .03f;
            text.anchor = TextAnchor.MiddleCenter;
            text.alignment = TextAlignment.Center;
            text.color = color;
            var renderer = go.GetComponent<Renderer>();
            renderer.sharedMaterial = font.material;
            var bounds = renderer.localBounds.size;
            if (bounds.x > .001f && bounds.y > .001f) text.characterSize *= Mathf.Min(size.x / bounds.x, size.y / bounds.y);
            return go;
        }

        static GameObject Model(string asset, Vector3 position, float yaw = 0f, float scale = 1f)
        {
            var source = AssetDatabase.LoadAssetAtPath<GameObject>(Folder + asset + ".fbx");
            if (!source) throw new FileNotFoundException("Chapter09 requires the executed Blender export: " + asset);
            var instance = (GameObject)PrefabUtility.InstantiatePrefab(source);
            instance.name = asset;
            instance.transform.SetParent(root, true);
            instance.transform.position = position;
            instance.transform.rotation = Quaternion.Euler(0, yaw, 0);
            instance.transform.localScale = Vector3.one * scale;
            foreach (var renderer in instance.GetComponentsInChildren<Renderer>())
            {
                var materials = renderer.sharedMaterials;
                for (int i = 0; i < materials.Length; i++)
                {
                    var key = materials[i] ? materials[i].name.Split('.')[0] : "CH09_Green";
                    materials[i] = surfaces.TryGetValue(key, out var authored) ? authored : M("Green");
                }
                renderer.sharedMaterials = materials;
            }
            return instance;
        }

        static Light Light(string name, Vector3 position, Color color, float intensity, float range, Vector3 target, float cone = 95)
        {
            var go = new GameObject(name);
            go.transform.SetParent(root, true);
            go.transform.position = position;
            go.transform.rotation = Quaternion.LookRotation(target - position);
            var light = go.AddComponent<Light>();
            light.type = LightType.Spot;
            light.color = color;
            light.intensity = intensity;
            light.range = range;
            light.spotAngle = cone;
            light.innerSpotAngle = cone * .6f;
            light.shadows = LightShadows.None;
            return light;
        }

        static Light InteriorFill(string name, Vector3 position, float intensity, float range)
        {
            var go = new GameObject(name);
            go.transform.SetParent(root, true);
            go.transform.position = position;
            var light = go.AddComponent<Light>();
            light.type = LightType.Point;
            light.color = new Color(1f, .91f, .76f);
            light.intensity = intensity;
            light.range = range;
            light.shadows = LightShadows.None;
            return light;
        }

        static void InteriorLighting()
        {
            // Candidate01 revealed two separate faults: downward spotlights
            // never lit upper walls, and pass06's sceneSaving hook overwrote the
            // inherited fixtures to .20/.45 intensity. Dress now runs pass06
            // first, then keeps this bounded rig as the final authored state.
            // The four-light-per-object URP budget and night ambient stay intact.
            var diffuser = Surface("CH09_ControlDiffuser", new Color(.88f, .87f, .72f), .8f, 0, new Color(.72f, .72f, .52f));
            foreach (var light in Object.FindObjectsByType<Light>(FindObjectsSortMode.None))
            {
                if (light.name == "FixtureLight")
                {
                    // The old point sources sat only .65m below the ceiling and
                    // clipped its values. Broad downward emission matches the
                    // fixture housing while preserving the useful desk light.
                    light.type = LightType.Spot;
                    light.transform.rotation = Quaternion.Euler(90, 0, 0);
                    light.spotAngle = 135f;
                    light.innerSpotAngle = 95f;
                    light.intensity = Mathf.Abs(light.transform.position.x) < 1f ? 1.70f : 1.45f;
                    light.range = 6.7f;
                    light.color = new Color(1f, .94f, .80f);
                    light.shadows = LightShadows.None;
                }
                else if (light.name == "AmberWorkLight06" || light.name == "CoolWorkLight06")
                {
                    light.color = light.name == "AmberWorkLight06" ? new Color(1f, .76f, .46f) : new Color(.77f, .86f, .83f);
                    Box("CH09.ControlTaskDiffuser", light.transform.position + new Vector3(0, .048f, 0), new Vector3(.37f, .017f, .15f), diffuser);
                }
            }
            foreach (var renderer in Object.FindObjectsByType<MeshRenderer>(FindObjectsSortMode.None))
                if (renderer.name == "Diffuser") renderer.sharedMaterial = diffuser;
            foreach (var label in Object.FindObjectsByType<TextMesh>(FindObjectsSortMode.None))
                if (label.name == "StatusText" && label.text.Contains("TRACK NOMINAL"))
                {
                    label.text = label.text.Replace("TRACK NOMINAL", "SCHEDULED TRACK");
                    var width = label.GetComponent<Renderer>().localBounds.size.x;
                    if (width > .70f) label.characterSize *= .70f / width;
                }
            InteriorFill("CH09.ControlRearWestFill", new Vector3(-4.2f, 2.38f, 4.2f), 1.30f, 5.6f);
            InteriorFill("CH09.ControlRearEastFill", new Vector3(4.2f, 2.38f, 4.2f), 1.30f, 5.6f);
            InteriorFill("CH09.ControlWorkFill", new Vector3(0, 2.20f, 1.3f), 1.10f, 5.0f);
            // Short ranges keep these fills local to the annex. They illuminate
            // vertical instrument cases and the processing card, while the
            // practical spots still establish distinct wet/archive work areas.
            InteriorFill("CH09.LabEnlargerFill", new Vector3(-1.80f, 2.48f, 10.30f), .95f, 4.5f);
            InteriorFill("CH09.LabArchiveFill", new Vector3(1.80f, 2.45f, 11.55f), .85f, 4.5f);
            InteriorFill("CH09.LabNorthFill", new Vector3(-.40f, 2.30f, 13.15f), .80f, 2.9f);
            Debug.Log("CHAPTER09_INTERIOR_LIGHTING warm local fills; pass06 fixtures restored; 4 additional lights/object retained; exterior ambient unchanged");
        }

        static GameObject Action(ChapterInvestigation investigation, string action, string label, Vector3 position, Vector3 size)
        {
            var go = new GameObject("CH09.Action." + action);
            go.transform.SetParent(root, true);
            go.transform.position = position;
            var box = go.AddComponent<BoxCollider>();
            box.size = size;
            var interaction = go.AddComponent<ChapterAction>();
            interaction.investigation = investigation;
            interaction.action = action;
            interaction.label = label;
            return go;
        }

        static void RetainDisabled(GameObject go)
        {
            if (!go) return;
            foreach (var renderer in go.GetComponentsInChildren<Renderer>()) renderer.enabled = false;
            foreach (var collider in go.GetComponentsInChildren<Collider>()) collider.enabled = false;
        }

        static void LaboratoryShell()
        {
            // Keep the original back wall as a disabled authoring reference. The
            // new opening is a genuine two-metre passage with floor continuity.
            RetainDisabled(GameObject.Find("BackWall"));
            Box("CH09.ControlRoomNorthWest", new Vector3(-5.175f, 1.725f, 7.5f), new Vector3(8.35f, 3.45f, .12f), M("UpperWall"), true);
            Box("CH09.ControlRoomNorthEast", new Vector3(5.175f, 1.725f, 7.5f), new Vector3(8.35f, 3.45f, .12f), M("UpperWall"), true);
            Box("CH09.LabDoorLintel", new Vector3(0, 2.975f, 7.5f), new Vector3(2f, .95f, .16f), M("UpperWall"), true);
            foreach (float x in new[] { -1.04f, 1.04f })
                Box("CH09.LabDoorJamb", new Vector3(x, 1.25f, 7.5f), new Vector3(.095f, 2.5f, .25f), M("Green"), true);
            Box("CH09.LabDoorFrameTop", new Vector3(0, 2.51f, 7.5f), new Vector3(2.18f, .10f, .25f), M("Green"), true);
            Box("CH09.LabThreshold", new Vector3(0, .003f, 7.5f), new Vector3(1.98f, .015f, .36f), M("Steel"));
            Box("CH09.LabEntrancePlate", new Vector3(0, 2.79f, 7.393f), new Vector3(1.82f, .33f, .04f), M("Green"));
            Label("CH09.LabEntranceText", "PHOTOGRAPHIC LAB\nPROCESS / EXAMINE / FILE", new Vector3(0, 2.79f, 7.367f), new Vector2(1.62f, .25f), Quaternion.identity, LabelInk);
            Label("CH09.LabExitText", "CONTROL ROOM", new Vector3(0, 2.75f, 7.597f), new Vector2(1.6f, .14f), Quaternion.Euler(0, 180, 0), LabelInk);

            // Segmented surfaces let URP select nearby practical lights for each
            // area within the established per-object additional-light budget.
            for (int x = 0; x < 4; x++) for (int z = 0; z < 4; z++)
                Box("CH09.LabFloor." + x + "." + z, new Vector3(-3 + x * 2, -.08f, 8.3125f + z * 1.625f), new Vector3(2, .16f, 1.625f), M("Linoleum"), true);
            foreach (float side in new[] { -1f, 1f })
            {
                for (int i = 0; i < 3; i++)
                {
                    float z = 8.5833f + i * 2.1667f;
                    Box("CH09.LabLowerWall", new Vector3(side * 4, .61f, z), new Vector3(.16f, 1.22f, 2.1667f), M("LowerWall"), true);
                    Box("CH09.LabUpperWall", new Vector3(side * 4, 2.21f, z), new Vector3(.16f, 1.98f, 2.1667f), M("UpperWall"), true);
                }
                Box("CH09.LabDadoRail", new Vector3(side * 3.895f, 1.22f, 10.75f), new Vector3(.06f, .07f, 6.5f), M("Trim"));
                Box("CH09.LabSkirting", new Vector3(side * 3.89f, .07f, 10.75f), new Vector3(.065f, .14f, 6.5f), M("Rubber"));
            }
            for (int x = 0; x < 4; x++)
            {
                Box("CH09.LabNorthLower", new Vector3(-3 + x * 2, .61f, 14), new Vector3(2, 1.22f, .16f), M("LowerWall"), true);
                Box("CH09.LabNorthUpper", new Vector3(-3 + x * 2, 2.21f, 14), new Vector3(2, 1.98f, .16f), M("UpperWall"), true);
            }
            Box("CH09.LabNorthDado", new Vector3(0, 1.22f, 13.892f), new Vector3(8, .07f, .06f), M("Trim"));
            Box("CH09.LabNorthSkirting", new Vector3(0, .07f, 13.89f), new Vector3(8, .14f, .065f), M("Rubber"));
            Box("CH09.LabCeiling", new Vector3(0, 3.23f, 10.75f), new Vector3(8.2f, .10f, 6.7f), M("UpperWall"), true);
            for (float x = -4; x <= 4; x++)
                Box("CH09.CeilingGrid", new Vector3(x, 3.165f, 10.75f), new Vector3(.018f, .012f, 6.5f), M("Trim"));
            for (float z = 7.5f; z <= 14; z += 1.08333f)
                Box("CH09.CeilingGrid", new Vector3(0, 3.165f, z), new Vector3(8, .012f, .018f), M("Trim"));
            Box("CH09.ExtractorHousing", new Vector3(-.1f, 3.10f, 13.2f), new Vector3(.56f, .11f, .50f), M("Green"));
            for (int i = 0; i < 7; i++)
                Box("CH09.ExtractorLouvre", new Vector3(-.31f + i * .07f, 3.036f, 13.2f), new Vector3(.028f, .017f, .40f), M("Rubber"));
            foreach (float x in new[] { -.85f, .85f })
            {
                Box("CH09.LabCeilingFixture", new Vector3(x, 3.08f, 9.4f), new Vector3(.20f, .12f, .92f), M("Steel"));
                Box("CH09.LabCeilingDiffuser", new Vector3(x, 3.012f, 9.4f), new Vector3(.14f, .025f, .79f), M("WarmDiffuser"));
            }
            Light("CH09.LabEntrancePractical", new Vector3(0, 2.98f, 9.0f), new Color(1f, .86f, .66f), 4.8f, 6f, new Vector3(0, .3f, 10.2f), 108);
            Light("CH09.LabSinkPractical", new Vector3(0, 2.90f, 13.2f), new Color(.92f, .94f, .79f), 3.2f, 4.9f, new Vector3(0, .7f, 12.3f), 105);
        }

        static void Bench(string name, float x, float z, float length)
        {
            Box(name + ".FormicaTop", new Vector3(x, .923f, z), new Vector3(1.16f, .072f, length), M("Cream"), true);
            Box(name + ".SteelApron", new Vector3(x, .82f, z), new Vector3(1.10f, .14f, length - .08f), M("Green"));
            Box(name + ".LowerShelf", new Vector3(x, .25f, z), new Vector3(1.04f, .05f, length - .12f), M("Green"));
            foreach (float xx in new[] { x - .47f, x + .47f }) foreach (float zz in new[] { z - length * .5f + .10f, z + length * .5f - .10f })
            {
                Box(name + ".TubularLeg", new Vector3(xx, .45f, zz), new Vector3(.047f, .9f, .047f), M("Steel"), true);
                Box(name + ".RubberFoot", new Vector3(xx, .025f, zz), new Vector3(.068f, .05f, .068f), M("Rubber"));
            }
            float face = x < 0 ? x + .585f : x - .585f;
            for (int i = 0; i < 3; i++)
            {
                float y = .37f + i * .17f;
                Box(name + ".DrawerFront", new Vector3(face, y, z - length * .5f + .40f), new Vector3(.034f, .146f, .61f), M("Cream"), true);
                Box(name + ".DrawerHandle", new Vector3(face + Mathf.Sign(-x) * .035f, y + .025f, z - length * .5f + .40f), new Vector3(.055f, .032f, .22f), M("Steel"));
            }
        }

        static void LaboratoryFurniture(ChapterInvestigation chapter)
        {
            Bench("CH09.WetBench", -2.77f, 11.25f, 4.15f);
            Bench("CH09.ArchiveBench", 2.77f, 11.25f, 4.15f);
            // The enlarger is a retained Blender source: visible condenser head,
            // rack scale, real folded bellows, lens, easel, knobs and power lead.
            Model("CH09_Enlarger", new Vector3(-2.85f, .963f, 10.30f), 90);
            Model("CH09_ProcessingTrays", new Vector3(-2.69f, .965f, 11.86f), 90);
            Label("CH09.WetBenchText", "01 / WET PROCESS\nDEVELOP  -  STOP  -  FIX", new Vector3(-2.165f, .80f, 11.58f), new Vector2(1.6f, .13f), Quaternion.Euler(0, 270, 0), LabelInk);
            // This unique material is the in-world wet print; ChapterInvestigation
            // can assign the actual frozen player exposure, never a stock image.
            var wetMaterial = Surface("CH09_DevelopingPrint", new Color(.81f, .80f, .68f), .35f);
            chapter.developingPrint = Box("CH09.DevelopingPrint", new Vector3(-2.66f, 1.014f, 11.86f), new Vector3(.27f, .003f, .35f), wetMaterial).GetComponent<Renderer>();
            Action(chapter, "develop", "USE PHOTOGRAPHIC WORKBENCH", new Vector3(-2.06f, 1.08f, 11.5f), new Vector3(.28f, .33f, 1.10f));

            Model("CH09_Sink", new Vector3(0, .79f, 13.46f), 180);
            foreach (float x in new[] { -.72f, .72f })
                Box("CH09.SinkLeg", new Vector3(x, .44f, 13.40f), new Vector3(.05f, .88f, .44f), M("Steel"), true);
            Box("CH09.SinkCollision", new Vector3(0, .86f, 13.45f), new Vector3(1.57f, .24f, .60f), M("Steel"), true).GetComponent<Renderer>().enabled = false;
            Rod("CH09.WaterSupply", new Vector3(-.50f, .24f, 13.87f), new Vector3(-.50f, 1.40f, 13.87f), .032f, M("Steel"));
            Rod("CH09.WaterSupply", new Vector3(.50f, .24f, 13.87f), new Vector3(.50f, 1.40f, 13.87f), .032f, M("Steel"));
            Rod("CH09.SinkCrossSupply", new Vector3(-.50f, 1.40f, 13.87f), new Vector3(.50f, 1.40f, 13.87f), .032f, M("Steel"));
            Box("CH09.ChemicalShelf", new Vector3(0, 1.80f, 13.65f), new Vector3(2.10f, .05f, .38f), M("Steel"));
            for (int i = 0; i < 5; i++)
            {
                float x = -.75f + i * .36f;
                Model("CH09_ChemicalBottle", new Vector3(x, 1.827f, 13.63f), 180, i == 2 ? 1.16f : 1);
                Label("CH09.ChemicalLabel", new[] { "DEV", "STOP", "FIX", "WASH", "WET" }[i], new Vector3(x, 1.951f, 13.546f), new Vector2(.079f, .043f), Quaternion.identity, PaperInk);
            }
            Box("CH09.SafeHandlingCard", new Vector3(-1.57f, 1.83f, 13.90f), new Vector3(.90f, .80f, .016f), M("InstructionPaper"));
            Label("CH09.SafeHandlingText", "PROCESSING\n\n1  LOAD\n2  TRANSFER\n3  COLLECT", new Vector3(-1.57f, 1.83f, 13.885f), new Vector2(.78f, .66f), Quaternion.identity, PaperInk);

            // One practical safelight casts a localized red pool, leaving archive
            // typography under a different, readable warm desk light.
            Rod("CH09.SafelightConduit", new Vector3(-3.885f, 1.32f, 12.35f), new Vector3(-3.885f, 2.54f, 12.35f), .025f, M("Steel"));
            Box("CH09.SafelightBracket", new Vector3(-3.65f, 2.47f, 12.35f), new Vector3(.43f, .05f, .05f), M("Steel"));
            Box("CH09.SafelightHood", new Vector3(-3.43f, 2.36f, 12.35f), new Vector3(.31f, .24f, .32f), M("Rubber"));
            Box("CH09.SafelightLens", new Vector3(-3.43f, 2.224f, 12.35f), new Vector3(.25f, .021f, .26f), M("RedDiffuser"));
            Light("CH09.WetBenchSafelight", new Vector3(-3.42f, 2.19f, 12.35f), new Color(1f, .26f, .13f), 1.45f, 3.4f, new Vector3(-2.57f, .91f, 11.85f), 96);
            Label("CH09.SafelightNotice", "SAFELIGHT AREA\nKEEP EXPOSED FILM CLOSED", new Vector3(-3.895f, 1.62f, 11.75f), new Vector2(.96f, .19f), Quaternion.Euler(0, 270, 0), LabelInk);

            // Empty clips and archival negatives establish the photographic task
            // without inserting a pre-made image into the player's evidence chain.
            Rod("CH09.DryingLine", new Vector3(-3.68f, 1.97f, 10.1f), new Vector3(-3.68f, 1.97f, 12.9f), .010f, M("Steel"));
            for (int i = 0; i < 6; i++)
            {
                float z = 10.3f + i * .44f;
                Box("CH09.DryingClip", new Vector3(-3.68f, 1.93f, z), new Vector3(.019f, .068f, .018f), M("Steel"));
                if (i % 2 == 0)
                {
                    Box("CH09.ArchiveNegativeStrip", new Vector3(-3.68f, 1.76f, z), new Vector3(.012f, .29f, .09f), M("Amber"));
                    for (int frame = 0; frame < 3; frame++)
                        Box("CH09.ArchiveNegativeFrame", new Vector3(-3.669f, 1.66f + frame * .083f, z), new Vector3(.003f, .065f, .065f), M("Rubber"));
                }
            }
            Model("CH09_LabStool", new Vector3(-2.15f, 0, 9.03f));
            Model("CH09_LabStool", new Vector3(2.15f, 0, 9.00f));
            Box("CH09.WetBenchRubberMat", new Vector3(-1.76f, .008f, 11.0f), new Vector3(.75f, .015f, 2.55f), M("Rubber"));
            Box("CH09.ArchiveRubberMat", new Vector3(1.76f, .008f, 11.0f), new Vector3(.75f, .015f, 2.55f), M("Rubber"));
            Archive(chapter);
        }

        static void Archive(ChapterInvestigation chapter)
        {
            Box("CH09.ArchiveLightboxBody", new Vector3(2.67f, .998f, 11.40f), new Vector3(.83f, .078f, 1.06f), M("Green"));
            Box("CH09.ArchiveLightboxGlass", new Vector3(2.67f, 1.043f, 11.4f), new Vector3(.72f, .012f, .94f), M("LightBox"));
            Label("CH09.ArchiveWorkface", "02 / EXAMINE\nREFERENCE / PHOTOGRAPHS", new Vector3(2.164f, .80f, 11.5f), new Vector2(1.64f, .15f), Quaternion.Euler(0, 90, 0), LabelInk);
            Action(chapter, "archive", "EXAMINE PHOTOGRAPHS & FIELD RECORDS", new Vector3(2.06f, 1.10f, 11.5f), new Vector3(.28f, .35f, 1.10f));
            Box("CH09.ReportClipboard", new Vector3(2.65f, .989f, 12.5f), new Vector3(.69f, .028f, .44f), M("Green"));
            Box("CH09.ReportPaper", new Vector3(2.63f, 1.008f, 12.5f), new Vector3(.60f, .009f, .37f), M("Paper"));
            Label("CH09.ReportHeading", "SARO / NIGHT REPORT\n\nOBSERVATION\nMETHOD\nCONCLUSION", new Vector3(2.63f, 1.015f, 12.50f), new Vector2(.49f, .28f), Quaternion.Euler(90, 90, 0), PaperInk);
            Action(chapter, "report", "FILE NIGHT INVESTIGATION REPORT", new Vector3(2.06f, 1.09f, 12.52f), new Vector3(.28f, .34f, .64f));

            Box("CH09.AtlasBoardFrame", new Vector3(3.884f, 1.99f, 11.80f), new Vector3(.085f, 1.40f, 2.32f), M("Green"));
            Box("CH09.AtlasBoardCork", new Vector3(3.834f, 1.99f, 11.80f), new Vector3(.028f, 1.29f, 2.21f), M("Cork"));
            Box("CH09.FieldPlanPaper", new Vector3(3.811f, 2.00f, 11.51f), new Vector3(.01f, 1.10f, 1.28f), M("InstructionPaper"));
            Label("CH09.FieldPlanTitle", "SARO / FIELD MAP", new Vector3(3.798f, 2.42f, 11.51f), new Vector2(1.12f, .13f), Quaternion.Euler(0, 90, 0), PaperInk);
            // Plan topology is built from native lines/text, not a decorative
            // generated map: room -> service yard -> S-03 -> B-12.
            Box("CH09.MapControlRoom", new Vector3(3.794f, 2.16f, 11.43f), new Vector3(.004f, .20f, .61f), M("Green"));
            Box("CH09.MapServiceRoute", new Vector3(3.794f, 1.96f, 11.08f), new Vector3(.004f, .34f, .035f), M("Green"));
            Box("CH09.MapS03Point", new Vector3(3.791f, 1.98f, 11.08f), new Vector3(.010f, .065f, .065f), M("Red"));
            Box("CH09.MapB12Point", new Vector3(3.791f, 1.78f, 11.08f), new Vector3(.010f, .065f, .065f), M("Red"));
            Label("CH09.MapControlText", "CONTROL ROOM", new Vector3(3.780f, 2.16f, 11.43f), new Vector2(.54f, .095f), Quaternion.Euler(0, 90, 0), LabelInk);
            Label("CH09.MapS03Text", "S-03 / MOTOR BUS", new Vector3(3.780f, 1.98f, 11.65f), new Vector2(.90f, .10f), Quaternion.Euler(0, 90, 0), PaperInk);
            Label("CH09.MapB12Text", "B-12 / OPTICAL", new Vector3(3.780f, 1.78f, 11.65f), new Vector2(.90f, .10f), Quaternion.Euler(0, 90, 0), PaperInk);
            Label("CH09.MapDistance", "EAST WALK\nB-12: SOUTH OF S-03", new Vector3(3.780f, 1.55f, 11.51f), new Vector2(1.05f, .16f), Quaternion.Euler(0, 90, 0), PaperInk);
            Box("CH09.ReferenceInspectionCard", new Vector3(3.810f, 1.98f, 12.47f), new Vector3(.01f, 1.10f, .55f), M("InstructionPaper"));
            Label("CH09.ReferenceInspectionText", "B-12\n\nONE VANE\nONE STRIPE\n\nCHECK\nBEFORE\nEXPOSURE", new Vector3(3.789f, 1.98f, 12.47f), new Vector2(.47f, .96f), Quaternion.Euler(0, 90, 0), PaperInk);

            Box("CH09.ArchiveDeskLampBase", new Vector3(3.23f, 1.004f, 10.67f), new Vector3(.23f, .08f, .28f), M("Green"));
            Rod("CH09.ArchiveLampArm", new Vector3(3.22f, 1.04f, 10.67f), new Vector3(3.22f, 1.63f, 10.55f), .025f, M("Steel"));
            Rod("CH09.ArchiveLampArm", new Vector3(3.22f, 1.63f, 10.55f), new Vector3(2.91f, 2.34f, 10.45f), .025f, M("Steel"));
            Box("CH09.ArchiveLampShade", new Vector3(2.91f, 2.36f, 10.45f), new Vector3(.31f, .10f, .29f), M("Green"));
            Box("CH09.ArchiveLampDiffuser", new Vector3(2.91f, 2.297f, 10.45f), new Vector3(.24f, .02f, .23f), M("WarmDiffuser"));
            Light("CH09.ArchiveTaskLight", new Vector3(2.91f, 2.27f, 10.45f), new Color(1f, .88f, .69f), 1.25f, 3.8f, new Vector3(3.81f, 1.96f, 11.65f), 105);
            for (int i = 0; i < 5; i++)
            {
                float z = 9.50f + i * .15f;
                Box("CH09.ArchiveBinder", new Vector3(3.02f, 1.19f, z), new Vector3(.32f, .44f, .115f), i % 2 == 0 ? M("Green") : M("Rubber"));
                Box("CH09.BinderLabel", new Vector3(2.852f, 1.19f, z), new Vector3(.009f, .12f, .060f), M("Paper"));
            }
            for (int i = 0; i < 2; i++)
            {
                Box("CH09.ArchiveBox", new Vector3(2.80f, .44f, 10.8f + i * 1.05f), new Vector3(.69f, .33f, .79f), M("Cork"));
                Box("CH09.ArchiveBoxLabel", new Vector3(2.448f, .47f, 10.8f + i * 1.05f), new Vector3(.01f, .12f, .26f), M("Paper"));
            }
            // Wall sockets and conduit explain practical lights and laboratory use.
            foreach (float x in new[] { -3.88f, 3.88f })
            {
                Rod("CH09.LabPowerConduit", new Vector3(x, 1.42f, 8.2f), new Vector3(x, 1.42f, 13.4f), .023f, M("Steel"));
                for (int i = 0; i < 3; i++)
                {
                    float z = 9.3f + i * 1.7f;
                    Box("CH09.LabOutlet", new Vector3(x, 1.44f, z), new Vector3(.042f, .16f, .10f), M("Cream"));
                    Box("CH09.LabOutletSocket", new Vector3(x - Mathf.Sign(x) * .029f, 1.455f, z), new Vector3(.012f, .042f, .037f), M("Rubber"));
                }
            }
        }

        static void FieldStation(ChapterInvestigation chapter)
        {
            for (int i = 0; i < 3; i++)
                Box("CH09.ServiceWalkExtension." + i, new Vector3(10.75f, 0, -15.75f - i * 1.5f), new Vector3(2.5f, .16f, 1.5f), M("Concrete"), true);
            for (int x = 0; x < 2; x++) for (int z = 0; z < 3; z++)
                Box("CH09.B12Pad." + x + "." + z, new Vector3(9.425f + x * 2.15f, 0, -19.667f - z * 1.833f), new Vector3(2.15f, .16f, 1.833f), M("Concrete"), true);
            for (int i = 0; i < 5; i++)
            {
                float z = -15.0f - i * 2.15f;
                Rod("CH09.B12RailPost", new Vector3(12.67f, .08f, z), new Vector3(12.67f, 1.14f, z), .055f, M("Steel"), true);
                Box("CH09.RailFoot", new Vector3(12.67f, .09f, z), new Vector3(.19f, .035f, .19f), M("Steel"));
            }
            Rod("CH09.B12EastHandrail", new Vector3(12.67f, 1.14f, -15), new Vector3(12.67f, 1.14f, -23.6f), .058f, M("Steel"), true);
            Rod("CH09.B12EastKneeRail", new Vector3(12.67f, .57f, -15), new Vector3(12.67f, .57f, -23.6f), .036f, M("Steel"));
            foreach (float z in new[] { -19f, -21.3f, -23.6f })
                Rod("CH09.B12WestRailPost", new Vector3(8.36f, .08f, z), new Vector3(8.36f, 1.14f, z), .055f, M("Steel"), true);
            Rod("CH09.B12WestHandrail", new Vector3(8.36f, 1.14f, -19), new Vector3(8.36f, 1.14f, -23.6f), .058f, M("Steel"), true);
            Rod("CH09.B12SouthHandrail", new Vector3(8.36f, 1.14f, -23.6f), new Vector3(12.67f, 1.14f, -23.6f), .058f, M("Steel"), true);
            Rod("CH09.B12SouthConduit", new Vector3(8.65f, .16f, -23.3f), new Vector3(12.45f, .16f, -23.3f), .036f, M("Steel"));
            Rod("CH09.B12CabinetConduit", new Vector3(12.30f, .16f, -23.3f), new Vector3(12.30f, .16f, -20.25f), .036f, M("Steel"));

            Rod("CH09.PathLampPole", new Vector3(12.10f, .08f, -17.2f), new Vector3(12.10f, 2.70f, -17.2f), .061f, M("Steel"));
            Rod("CH09.PathLampArm", new Vector3(12.10f, 2.70f, -17.2f), new Vector3(11.40f, 2.70f, -17.2f), .048f, M("Steel"));
            Box("CH09.PathLampHousing", new Vector3(11.40f, 2.65f, -17.2f), new Vector3(.32f, .16f, .31f), M("Rubber"));
            Box("CH09.PathLampDiffuser", new Vector3(11.40f, 2.56f, -17.2f), new Vector3(.25f, .02f, .24f), M("WarmDiffuser"));
            Light("CH09.PathLamp", new Vector3(11.40f, 2.53f, -17.2f), new Color(1f, .61f, .27f), 12, 6.4f, new Vector3(10.7f, .1f, -17.8f), 100);
            Box("CH09.B12RouteSign", new Vector3(12.59f, 1.0f, -17.40f), new Vector3(.055f, .36f, .73f), M("Green"));
            Label("CH09.B12RouteText", "B-12\nOPTICAL REFERENCE", new Vector3(12.549f, 1.0f, -17.40f), new Vector2(.64f, .27f), Quaternion.Euler(0, 90, 0), LabelInk);

            Model("CH09_FieldCabinet", new Vector3(11.67f, .08f, -20.40f));
            Box("CH09.B12CabinetCollider", new Vector3(11.67f, .67f, -20.40f), new Vector3(.72f, 1.18f, .49f), M("Green"), true).GetComponent<Renderer>().enabled = false;
            Label("CH09.B12CabinetIdentifier", "B-12", new Vector3(11.52f, .94f, -20.139f), new Vector2(.30f, .14f), Quaternion.Euler(0, 180, 0), LabelInk);
            Label("CH09.B12CabinetModeLegend", "SHIELD / DRIVE\nREFERENCE 042", new Vector3(11.66f, .76f, -20.126f), new Vector2(.52f, .14f), Quaternion.Euler(0, 180, 0), LabelInk);
            Action(chapter, "experiment", "OPERATE B-12 OPTICAL REFERENCE", new Vector3(11.66f, 1.02f, -20.10f), new Vector3(.80f, .84f, .20f));

            // A physical optical reference has one real stripe. The actual
            // photographic render briefly includes a second, fixed mesh. The
            // exposure code freezes that rendered image and metadata at capture.
            var pivot = new GameObject("CH09.ReferenceVane");
            pivot.transform.SetParent(root, true);
            pivot.transform.position = new Vector3(10, 1.60f, -22);
            chapter.referenceVane = pivot.transform;
            ReferenceBoard(pivot.transform, false);
            var echo = new GameObject("CH09.PhotoEcho");
            echo.transform.SetParent(root, true);
            echo.transform.position = new Vector3(10.45f, 1.60f, -22.012f);
            ReferenceBoard(echo.transform, true);
            chapter.photoEcho = echo;
            echo.SetActive(false);

            Rod("CH09.ReferenceAxle", new Vector3(10, .69f, -22), new Vector3(10, 1.34f, -22), .071f, M("Steel"));
            for (int i = 0; i < 3; i++)
            {
                float angle = (30 + i * 120) * Mathf.Deg2Rad;
                var foot = new Vector3(10 + Mathf.Sin(angle) * .46f, .12f, -22 + Mathf.Cos(angle) * .46f);
                Rod("CH09.ReferenceTripodLeg", new Vector3(10, .94f, -22), foot, .041f, M("Steel"));
                Box("CH09.ReferenceTripodFoot", foot, new Vector3(.18f, .032f, .16f), M("Steel"));
            }
            Box("CH09.ReferenceSafetyCollider", new Vector3(10, .57f, -22), new Vector3(.52f, 1f, .52f), M("Green"), true).GetComponent<Renderer>().enabled = false;
            Rod("CH09.ReferenceLampPole", new Vector3(9.09f, .08f, -21.72f), new Vector3(9.09f, 2.17f, -21.72f), .044f, M("Steel"));
            Rod("CH09.ReferenceLampNeck", new Vector3(9.09f, 2.17f, -21.72f), new Vector3(9.35f, 2.31f, -21.72f), .035f, M("Steel"));
            var hood = Box("CH09.ReferenceLampShroud", new Vector3(9.35f, 2.25f, -21.72f), new Vector3(.29f, .27f, .26f), M("Green"));
            hood.transform.rotation = Quaternion.Euler(0, 0, 30);
            Box("CH09.ReferenceLampDiffuser", new Vector3(9.46f, 2.135f, -21.72f), new Vector3(.16f, .021f, .18f), M("WarmDiffuser"));
            chapter.experimentLight = Light("CH09.ExperimentLight", new Vector3(9.49f, 2.13f, -21.72f), new Color(1f, .65f, .31f), 9.8f, 5.3f, new Vector3(10.30f, 1.24f, -22), 110);
            Light("CH09.B12ReadLight", new Vector3(12.17f, 2.7f, -20.55f), new Color(.70f, .79f, .84f), 4.0f, 4.0f, new Vector3(11.55f, .85f, -20.10f), 78);
            Box("CH09.B12ReadLightHood", new Vector3(12.17f, 2.75f, -20.55f), new Vector3(.24f, .12f, .20f), M("Rubber"));
            Rod("CH09.B12ReadLightPole", new Vector3(12.17f, .08f, -20.55f), new Vector3(12.17f, 2.74f, -20.55f), .039f, M("Steel"));
            // A visible sighting mark changes the second visit from another
            // antenna photograph into an intentional close control experiment.
            Box("CH09.B12SightingMark", new Vector3(10.55f, .086f, -19.3f), new Vector3(.62f, .012f, .06f), M("Stripe"));
            Box("CH09.B12SightingCross", new Vector3(10.55f, .086f, -19.3f), new Vector3(.06f, .012f, .49f), M("Stripe"));
            Label("CH09.B12SightingText", "B-12 / SIGHT LINE", new Vector3(10.55f, .099f, -18.95f), new Vector2(1.05f, .095f), Quaternion.Euler(90, 180, 0), LabelInk);
        }

        static void ReferenceBoard(Transform parent, bool photographic)
        {
            LocalBox(parent, photographic ? "CH09.FilmOnlyBacking" : "CH09.ReferenceBacking", Vector3.zero, new Vector3(.38f, 1.44f, .045f), M("Rubber"));
            LocalBox(parent, "CH09.ReferenceRimLeft", new Vector3(-.195f, 0, 0), new Vector3(.014f, 1.47f, .060f), M("Steel"));
            LocalBox(parent, "CH09.ReferenceRimRight", new Vector3(.195f, 0, 0), new Vector3(.014f, 1.47f, .060f), M("Steel"));
            LocalBox(parent, photographic ? "CH09.FilmOnlyStripe" : "CH09.PhysicalSingleStripe", new Vector3(.025f, .05f, .031f), new Vector3(.115f, 1.17f, .010f), photographic ? M("FilmStripe") : M("Stripe"));
            for (int i = 0; i < 11; i++)
                LocalBox(parent, "CH09.ReferenceGraduation", new Vector3(-.125f, -.55f + i * .112f, .030f), new Vector3(i % 5 == 0 ? .072f : .041f, .012f, .011f), M("Paper"));
            if (!photographic)
            {
                var text = Label("CH09.ReferencePlateText", "B-12", parent.position + new Vector3(0, -.646f, .04f), new Vector2(.22f, .067f), Quaternion.Euler(0, 180, 0), LabelInk, parent);
                text.transform.localPosition = new Vector3(0, -.646f, .04f);
                LocalBox(parent, "CH09.ReferenceOperatingHandle", new Vector3(.29f, -.54f, -.06f), new Vector3(.22f, .035f, .035f), M("Steel"));
                LocalBox(parent, "CH09.ReferenceHandleGrip", new Vector3(.40f, -.54f, -.06f), new Vector3(.041f, .16f, .065f), M("Rubber"));
            }
        }

        public static void Dress(GameSession session)
        {
            if (GameObject.Find("Chapter09Environment")) return;
            // Make the older save-time pass explicit before applying the
            // chapter's lighting, so its deferred hook cannot undo this rig.
            if (!GameObject.Find("ControlRoomArt06")) ControlRoomPass06.Dress();
            Directory.CreateDirectory(Folder);
            if (!AssetDatabase.IsValidFolder(Folder.TrimEnd('/'))) AssetDatabase.Refresh();
            Materials();
            font = AssetDatabase.LoadAssetAtPath<Font>("Assets/Signal47/Art/ThirdParty/VT323/VT323-Regular.ttf");
            root = new GameObject("Chapter09Environment").transform;
            if (!session.chapter) session.chapter = session.gameObject.AddComponent<ChapterInvestigation>();
            LaboratoryShell();
            LaboratoryFurniture(session.chapter);
            FieldStation(session.chapter);
            InteriorLighting();
            // Avoid permanently batching the moving vane or the capture-only
            // duplicate; all fixed architecture and furnishings are batchable.
            foreach (var renderer in root.GetComponentsInChildren<Renderer>(true))
            {
                if (renderer.transform.IsChildOf(session.chapter.referenceVane) || renderer.transform.IsChildOf(session.chapter.photoEcho.transform) || renderer == session.chapter.developingPrint) continue;
                GameObjectUtility.SetStaticEditorFlags(renderer.gameObject, StaticEditorFlags.BatchingStatic);
            }
            EditorUtility.SetDirty(session.chapter);
            Debug.Log("CHAPTER09_ENVIRONMENT_BUILT lab=[-4..4,7.5..14] door=2m; B-12=(10,1.6,-22); original Blender meshes + retained CC0 linoleum and VT323 labels");
        }
    }
}
#endif
