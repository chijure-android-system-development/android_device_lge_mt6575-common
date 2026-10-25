/*
 * Stub for libbessound_mtk.so — disables MTK BeSS audio enhancement.
 * The real library has a use-after-free race condition with AOSP AudioFlinger.
 * All functions return 0; audio still works without BeSS post-processing.
 *
 * BLOUD_SetHandle is special: it populates a 48-byte function-pointer table
 * that AudioCompensationFilter::Init() calls through immediately after.
 * All 12 slots are filled with a safe no-op to prevent NULL-ptr crashes.
 */

static int _bloud_noop(void) { return 0; }

int BLOUD_SetHandle(void *handle, void *param1, void *param2) {
    if (handle) {
        unsigned int *h = (unsigned int *)handle;
        int i;
        for (i = 0; i < 12; i++)
            h[i] = (unsigned int)(void *)_bloud_noop;
    }
    return 0;
}

int AEQ_Activate(void) { return 0; }
int AEQ_Limiter_NonBlockDelay(void) { return 0; }
int aeq_nolimiter(void) { return 0; }
int AEQ_Process(void) { return 0; }
int AEQ_SetMag(void) { return 0; }
int Apply_Limiter(void) { return 0; }
int AudioPP_Reverb_ApplyCoeff(void) { return 0; }
int BBAS_Open(void) { return 0; }
int BBAS_SetHandle(void) { return 0; }
int BCF_Close(void) { return 0; }
int BCF_GetBufferSize(void) { return 0; }
int BCF_Open(void) { return 0; }
int BCF_SetHandle(void) { return 0; }
int BEQ_Close(void) { return 0; }
int BEQ_Get_Band_Num(void) { return 0; }
int BEQ_GetBufferSize(void) { return 0; }
int BEQ_Internal_SetParameters(void) { return 0; }
int BEQ_Open(void) { return 0; }
int BEQ_SetHandle(void) { return 0; }
int BEQ_Transform_Mag_To_Band_Num(void) { return 0; }
int BEQ_Transform_Mag_To_Bar_Num(void) { return 0; }
int BHarmonic_SetHandle(void) { return 0; }
int BHDP_Close(void) { return 0; }
int BHDP_GetBufferSize(void) { return 0; }
int BHDP_Open(void) { return 0; }
int BHDP_SetHandle(void) { return 0; }
int BLIVE_SetHandle(void) { return 0; }
int BLIVE_SetParameters(void) { return 0; }
int BS_ActivateFadeIn(void) { return 0; }
int BS_ActivateFadeOut(void) { return 0; }
int BS_FadeInOut(void) { return 0; }
int BSRD_Internal_SetParameters(void) { return 0; }
int BSRD_Open(void) { return 0; }
int BSRD_SetHandle(void) { return 0; }
int BTS_Open(void) { return 0; }
int BTS_SetHandle(void) { return 0; }
int cal_A2(void) { return 0; }
int Calculate_Gain_Buf(void) { return 0; }
int cal_P(void) { return 0; }
int cal_S(void) { return 0; }
int DH_HighFreqAcc(void) { return 0; }
int DH_HighFreqAccCorePwr(void) { return 0; }
int DH_UpdateWsGainInQn(void) { return 0; }
int DH_WSGainProfileArrange(void) { return 0; }
int DH_WSTargetGainDecide(void) { return 0; }
int drvb_crack_heap(void) { return 0; }
int drvb_crash_heap(void) { return 0; }
int drvb_f0(void) { return 0; }
int drvb_gettimeofday(void) { return 0; }
int dump_buf(void) { return 0; }
int FL_HighShelvingFilter(void) { return 0; }
int FL_HighShelvingFilter2(void) { return 0; }
int FL_HighShelvingFilter2Core(void) { return 0; }
int FL_HighShelvingFilterMonoCore(void) { return 0; }
int FL_HighShelvingFilterStereoCore(void) { return 0; }
int FL_Order2IIRFilter(void) { return 0; }
int GA_DRCGainMapCore_DB(void) { return 0; }
int Get_Frame_Max(void) { return 0; }
int Get_Frame_Max_Index(void) { return 0; }
int get_heap_offset(void) { return 0; }
int get_ul(void) { return 0; }
int h23gen_get_mem_size(void) { return 0; }
int h23gen_init(void) { return 0; }
int h23gen_process(void) { return 0; }
int hash_finish(void) { return 0; }
int hash_starts(void) { return 0; }
int hash_update(void) { return 0; }
int No_Limiter(void) { return 0; }
int Non_Block_Delay_Limiter(void) { return 0; }
int parse_BEQ_Init_Param(void) { return 0; }
int Sample_Gain_Multiply(void) { return 0; }
int set_ul(void) { return 0; }
int sha1(void) { return 0; }
int TD_PreProcess(void) { return 0; }
int Tr_InputToQnCore(void) { return 0; }
int WS_WaveShapingMonoCore(void) { return 0; }
int WS_WaveShapingMonoCoreOutQn(void) { return 0; }
int WS_WaveShapingOutQn(void) { return 0; }
int XCorr(void) { return 0; }
