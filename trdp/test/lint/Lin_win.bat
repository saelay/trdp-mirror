"c:\Lint\Lint-nt" +v -i"c:\Lint" std_win.lnt -DWIN32;MD_SUPPORT=1 -os(_LINT.TMP) trdp_mdcom.c trdp_if.c trdp_pdcom.c trdp_marshall.c trdp_utils.c trdp_stats.c vos_utils.c vos_mem.c vos_thread.c vos_sock.c
type _LINT.TMP | more
@echo off
echo ---
echo  output placed in _LINT.TMP
echo --- 
