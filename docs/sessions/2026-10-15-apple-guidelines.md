# 2026-10-15: the window against Apple's guidelines

Asked: Transient Shuffler had just been made to follow Apple's design
principles; do the same for Noterator and Miderator, from the nineteen
Foundations pages of the Human Interface Guidelines the user attached
(June 2026). Done in both apps on `claude/blissful-bohr-lqwr1j`, as decision
0048, with the review page by page in `docs/apple-hig-review.md`.

## The route

1. Read every page, then both apps built (Debug), rendered and screenshotted
   headless before anything changed. Transient Shuffler's own review
   (its `docs/apple-hig-review.md`) set the family's answers: secondary text
   `#B0B6C0`, error text `#FF9E96`, a focus ring in the text colour.
2. Contrast measured with WCAG's formula for every text colour on every
   ground. Secondary text was 4.7:1, the white page's bar numbers 3.1:1, a
   generator's error in the house red 3.4:1. Raised to 7:1 or more (bar
   numbers 6.6:1); the theme's secondary colours became variables so the
   Mac's Increase Contrast can lift them (`useIncreasedContrast`).
3. Increase Contrast and Reduce Motion read from NSWorkspace with
   `objc_msgSend` from plain C++ (`SystemSettings.cpp`), polled every 1.5 s.
   Reduce Motion makes Follow turn pages; the user's own choice is kept.
4. The keyboard. Clicking a button left the keyboard on it - Return pressed
   it again rather than starting note input. `clicksKeepFocus` makes
   buttons, menus and sliders leave it with the score, as on the Mac. Tab
   and Shift+Tab, a ring in `focusRing`, Space presses, Esc returns.
5. VoiceOver names for everything (the test found the gaps: "M", "^", "-",
   "degree", unnamed setting menus), the page described in words, status
   messages announced.
6. The status line moved under the toolbar; Add Instrument, Blocks' buttons
   and the Generate message away from the bottom edge.
7. Interface Size 100-200% with `Desktop::setGlobalScaleFactor`; the toolbar
   folds into two rows. Tried at 200% in a 1600 by 1000 screen: the Generate
   tab squeezed its results out, so its description now gives way in a
   short panel and the settings scroll instead.
8. The Mac's alerts (`NativeMessageBox`) for questions and errors; the title
   bar as a document's (`setDocumentEditedStatus`, `setRepresentedFile`);
   title case and ellipses; drawn chevrons, cross, Play and Stop; out of
   range notes marked as well as red.
9. `Tests/TestWindow.cpp` (6 tests), then the same in Miderator.

## What looked broken and was not, and what was

- **Choosing a side tab handed the keyboard to the first button in it**
  (seen as a ring on Blocks' Chord after clicking the Blocks tab). JUCE's
  `TabbedComponent` calls `toFront (true)` on the new panel. Now the window
  takes the keyboard back in `currentTabChanged`.
- **No ring after Tab**, three times over, each a different cause:
  1. JUCE's buttons do not repaint when they gain or lose the keyboard; a
     focus listener repaints them.
  2. JUCE's own Tab order went from the window's content to the window
     itself and stopped. Tab is now the window's own: the score, every
     control, the score again.
  3. As the window opened, the keyboard went to the window, not the score
     (the score had it only after a click). The window now hands it on in
     `focusGained`. It may not happen on a Mac, where the window opens
     active; it did on Linux, and the fix is right either way.
- **Keys that went nowhere** in the headless tests: with no window manager,
  X sends keys to the window under the pointer, and a tooltip window counts.
  The pointer has to rest somewhere without a tooltip (the toolbar's gap).
- The first test run failed on the toolbar: it relied on the window to make
  its buttons keep the keyboard off them. Each panel now does it for itself.

## Not done yet

- Nothing of 0048 tried on a Mac: not VoiceOver, Full Keyboard Access,
  Increase Contrast or Reduce Motion, nor the native alerts and title bar.
  `SystemSettings.cpp` calls the Objective-C runtime and is only compiled
  for real on the Mac (CI).
- The app icon in Apple's layered Icon Composer format.
- VoiceOver reading notes one by one.
- Interface Size at 200% is cramped in a small window: the Generate tab
  shows two or three results.
