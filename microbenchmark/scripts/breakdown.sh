echo -------- SFI ----------
bin/sfi_pingpong
echo -------- SFI ---------

echo ------- ECALL --------
lps-go bin/ecall_loop -dasics
echo ------- ECALL --------

echo ------- PIPE --------
lps-pipe bin/ping bin/pong -dasics
echo ------- PIPE --------
