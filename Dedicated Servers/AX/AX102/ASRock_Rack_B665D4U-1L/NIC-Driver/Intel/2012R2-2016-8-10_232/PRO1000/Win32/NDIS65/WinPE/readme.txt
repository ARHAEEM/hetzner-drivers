Setting up Intel(R) 10GbE Network Connection Drivers for Microsoft* Windows* PE
===============================================================================
To set up network drivers for Microsoft* Windows* PE, you must import files from
the Intel(R) Network Connections Software CD to the Windows PE image. Follow the
directions for your operating system.
 
32-bit Microsoft Windows PE V6
==============================
1. Create a temporary directory (for example, c:\temp\winpe).
 
2. Copy all files from the PROXGB\Win32\NDIS65\WinPE directory to the temporary
   directory.
 
3. Extract the driver inf file (for example: ixe6532.inf) from
   PROXGB\Win32\NDIS65\WinPE\*.zip to the temporary directory.
 
4. Use the /Add-Driver option of the DISM.exe tool to import the drivers into
   the image. For example, use the following command:
 
   dism /image:C:\mounted_winpe /Add-Driver /driver:c:\temp\winpe\ixe6532.inf
 
The syntax for the dism.exe tool is as follow:
 
   dism /image:<%WINPEDIR%> /Add-Driver /driver:<path>
 
     /image:<%WINPEDIR%> specifies the path to the directory where you have
     mounted your Windows PE image.
     /Add-Driver specifices the action dism should take.
     /driver:<path> specifies the path to the driver inf file.
 
Please see your Microsoft Windows PE documentation or the Microsoft website for
more information on using dism.exe
 
x64 Microsoft Windows PE V6
===========================
1. Create a temporary directory (for example, c:\temp\winpe).
 
2. Copy all files from the PROXGB\Winx64\NDIS65 directory to the temporary
   directory. Do NOT copy the ixe65x64.inf file to this directory.
 
3. Extract the driver inf file (for example: ixe65x64.inf)from 
   PROXGB\Winx64\NDIS62\WinPE\*.zip to the temporary directory.
 
4. Use the /Add-Driver option of the DISM.exe tool to import the drives into
   the image. For example, use the following command:
 
   dism /image:C:\mounted_winpe /Add-Driver /driver:c:\temp\winpe\ixe62x64.inf
 
The syntax for the dism.exe tool is as follow:
 
   dism /image:<%WINPEDIR%> /Add-Driver /driver:<path>
 
     /image:<%WINPEDIR%> specifies the path to the directory where you have
     mounted your Windows PE image.
     /Add-Driver specifices the action dism should take.
     /driver:<path> specifies the path to the driver inf file.
 
Please see your Microsoft Windows PE documentation or the Microsoft website
for more information on using dism.exe
 
 
Known Issues
============
 
Boot failure in WinPE for Microsoft* Windows* 10
------------------------------------------------
Your system may fail to boot under WinPE for Microsoft Windows 10. This can
occur during a network install using IPv6. To resolve the issue, modify the
"Wait for Link" section of the INF that you insert into the boot image. In
the line
 
  HKR, Ndi\Params\WaitAutoNegComplete,      default,     0, "2"
 
Change the default "2"(auto detect) to "1" (wait for auto negotiation to
complete). This section of your INF file should look like the following example:
 
  ; Wait for Link
  HKR, Ndi\Params\WaitAutoNegComplete,      ParamDesc,   0, %WaitAutoNegComplete%
  HKR, Ndi\Params\WaitAutoNegComplete,      default,     0, "1"
  HKR, Ndi\Params\WaitAutoNegComplete\Enum,   "0",       0, %Off%
  HKR, Ndi\Params\WaitAutoNegComplete\Enum,   "1",       0, %On%
  HKR, Ndi\Params\WaitAutoNegComplete\Enum,   "2",       0, %AutoDetect%
  HKR, Ndi\Params\WaitAutoNegComplete,       type,       0, "enum"
 
Legal / Disclaimers
===================
Copyright (C) 2009-2015, Intel Corporation. All rights reserved.
 
Intel Corporation assumes no responsibility for errors or omissions in this
document. Nor does Intel make any commitment to update the information
contained herein.
 
Intel and the Intel logo are trademarks of Intel Corporation or its
subsidiaries in the U.S. and/or other countries.
 
*Other names and brands may be claimed as the property of others.
