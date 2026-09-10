using UnityEngine;
using Signal47.Core;

namespace Signal47.Audio
{
    /// <summary>Distance-driven foley; blocked input, menus and restored saves produce no steps.</summary>
    [RequireComponent(typeof(CharacterController))]
    public sealed class SurfaceFootsteps : MonoBehaviour
    {
        public AudioClip[] clips;
        AudioSource source;
        CharacterController controller;
        Vector3 previous;
        float distance;
        int index;
        void Awake()
        {
            controller = GetComponent<CharacterController>(); previous = transform.position;
            source = gameObject.AddComponent<AudioSource>(); source.playOnAwake = false; source.spatialBlend = 0;
        }
        void LateUpdate()
        {
            var current = transform.position; var delta = current - previous; delta.y = 0; previous = current;
            var g = GameSession.Instance;
            if (!g || !g.CanControl || !controller.isGrounded || delta.magnitude > .5f)
            { distance = 0; return; }
            if (clips == null || clips.Length == 0) return;
            distance += delta.magnitude;
            if (distance < 1.32f) return;
            distance %= 1.32f;
            bool exterior = current.x > 9.5f;
            source.pitch = (exterior ? .91f : 1.01f) + (index % 3 - 1) * .035f;
            source.PlayOneShot(clips[index++ % clips.Length], exterior ? .19f : .11f);
        }
    }
}
