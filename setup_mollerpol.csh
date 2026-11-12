# From the Mollerpol source directory: source setup_mollerpol.csh [build [ROOT_PREFIX]].
# Bash performs the shared build/setup; source its settings in the caller.
set _mollerpol_ok = 0
set _mollerpol_env = `mktemp`
if ($status != 0) goto mollerpol_done
bash "$cwd/setup_mollerpol.sh" --csh $argv:q >! "$_mollerpol_env"
if ($status == 0) then
    source "$_mollerpol_env"
    if ($status == 0) set _mollerpol_ok = 1
endif
/bin/rm -f "$_mollerpol_env"
mollerpol_done:
unset _mollerpol_env
eval "unset _mollerpol_ok; /bin/test $_mollerpol_ok = 1"
