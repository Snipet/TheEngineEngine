// tests/ui/RefTests.inc: the reference's own panel (sim/ref/panel.gui and its shaders, as the build embedded them),
// included by Tests.cpp. Skipped (77) in a build made before sim/ref/ was in the tree.
int refPanel()
{
    const Sources files = refPanelFiles();
    if (files.empty())
    {
        std::printf("  sim/ref/ was not in the tree when this was built: the reference's panel is not tested\n");
        return kSkip;
    }
    return 0;
}
