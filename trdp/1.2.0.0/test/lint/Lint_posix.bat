echo off
del _LINT_POS.TMP
C:\Programme\Lint\lint-nt.exe +v -iC:\Programme\Lint\lnt std_posix.lnt -DPOSIX;MD_SUPPORT=1;B_ENDIAN;__LITTLE_ENDIAN__=0;__BIG_ENDIAN__=1 -os(_LINT_POS.TMP) vos_mem.c vos_utils.c posix/vos_thread.c posix/vos_sock.c posix/vos_shared_mem.c trdp_if.c trdp_mdcom.c trdp_pdcom.c trdp_utils.c trdp_stats.c tau_marshall.c tau_ctrl.c tau_dnr.c tau_tti.c
REM type _LINT_POS.TMP | more
@echo off
echo ---
echo  output placed in _LINT_POS.TMP
echo --- 
