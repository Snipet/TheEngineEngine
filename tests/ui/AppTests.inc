// tests/ui/AppTests.inc: J2's display.tail (the source's newest samples in a panel's scope and spectrum). J2's frame
// tests moved with the frame to tests/ui/views/FrameTests.cpp (package U1). Included by Tests.cpp.

    // `scope out` and `spectrum out` read the source's newest samples (TelemetrySource::outputTail) when the panel's
    // owner gives none: silent, the frame stays; a tone in the tail, the two displays move.
    TEST(testDisplayTail, "display.tail")
    {
        // Two panels made and ticked alike, one with a tone in its source's tail: only the tail differs between them.
        const auto frameWith = [](double hz) {
            Rig rig;
            rig.player.tone = hz;
            rig.page("DISPLAYS");
            rig.host.tick(60);
            return funkgui::fingerprint(rig.host.draw()).geometry;
        };
        const uint64_t silent = frameWith(0.0), low = frameWith(440.0), high = frameWith(3000.0);
        CHECK(frameWith(0.0) == silent);                 // deterministic
        CHECK(low != silent && high != silent && high != low);
        // The reference's own panel, as makeMainPanel makes it, reads it too.
        const auto mainWith = [](double hz) {
            MockPlayer player;
            player.tone = hz;
            std::unique_ptr<funkgui::Panel> main = tee::ui::makeMainPanel(player, tee::AppServices{});
            funkgui::HeadlessHost host(*main, 0, 2.0f);
            host.tick(60);
            return funkgui::fingerprint(host.draw()).geometry;
        };
        CHECK(mainWith(440.0) != mainWith(0.0));
        return 0;
    }
