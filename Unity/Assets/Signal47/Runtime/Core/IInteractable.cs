namespace Signal47.Core
{
    public interface IInteractable
    {
        string Prompt { get; }
        void Interact();
    }
}
