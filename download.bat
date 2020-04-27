mkdir assets
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min8.exr'      '%~dp0assets/8.exr'"
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min64.exr'     '%~dp0assets/64.exr'"
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min512.exr'    '%~dp0assets/512.exr'"
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min4096.exr'   '%~dp0assets/4096.exr'"
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min32768.exr'  '%~dp0assets/32768.exr'"
powershell "Import-Module BitsTransfer; Start-BitsTransfer 'https://cg.ivd.kit.edu/publications/2018/rwmc/tool/bathroom/prog.dbg.2048.light_min262144.exr' '%~dp0assets/262144.exr'"
