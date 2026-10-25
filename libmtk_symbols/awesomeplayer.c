/*
 * MTK's AwesomePlayer hands its own pointer to libMtkOmxVdec through a
 * vendor extension, and the decoder then calls
 * AwesomePlayer::mtk_omx_get_current_time() for its frame-drop logic.
 * CM's player never passes that pointer, so the decoder skips the call
 * (it checks for NULL first); the symbol only has to resolve for the
 * library to load.
 */
void mtk_omx_get_current_time(void *player, long long *time_us)
        __asm__("_ZN7android13AwesomePlayer24mtk_omx_get_current_timeEPx");

void mtk_omx_get_current_time(void *player, long long *time_us)
{
    (void)player;
    if (time_us)
        *time_us = 0;
}
