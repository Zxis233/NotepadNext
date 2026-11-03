# Log monitoring smoke tests

Run these checks with a built application and a disposable log file. Log
monitoring is selected in **File > Log Monitoring** for the current file.

1. Open a file containing more lines than fit in the editor. Enable monitoring
   and append lines from another process while the application is active, in
   the background, and while another tab is selected. The document should
   update without a reload question, and its last display line should remain
   near three quarters of the editor height.
2. Scroll up, append more lines, and verify that the new text is loaded while
   the view stays at the previous position. Scroll down to the end and verify
   that following resumes. Repeat after truncating the file while paused.
3. Change the window size, zoom, and word wrap while following. Also check a
   long wrapped last line and a short file. The last line should remain near
   the target where the available scroll range allows it.
4. Edit a file before enabling monitoring. Cancel the save prompt and check
   that the edits remain and monitoring stays disabled; then enable monitoring
   and save when prompted. Editing should be blocked until monitoring is
   disabled. Repeat with a file that was already read-only.
5. With unsaved edits in the editor, change or replace the disk file before
   enabling monitoring. Confirm that the editor refuses to overwrite the disk
   file and retains the unsaved content.
6. Rename the monitored file away, wait for the unavailable status, and
   create a new file at the original path. The old view should remain intact
   until the replacement can be read, and then update without a modal prompt.
   Repeat with an atomic replacement and with a truncation.
7. Test UTF-8, UTF-16, and GBK files. Temporarily leave a multibyte character
   incomplete during an external write, then finish it. The old content should
   remain until decoding succeeds, without repeated error dialogs.
8. Monitor two files, switch between their tabs, disable monitoring for one,
   and close the other. Verify the menu check mark follows the active file and
   normal external-change prompts return for the file whose monitoring ended.
