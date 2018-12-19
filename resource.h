#define	IDC_STATIC						-1

#define	IDD_DIALOG1						101
#define	IDD_DIALOG2						102
#define	IDD_DIALOG3						103
#define	IDD_DIALOG4						104
#define	IDD_DIALOG5						105
#define	IDD_DIALOG6						106
#define	IDD_DIALOG7						107
#define	IDD_DIALOG8						108
#define    IDD_DIALOG9                     109
#define    IDD_DIALOG10                    110
#define    IDD_DIALOG11                    111
#define    IDD_DIALOG12                    112
#define    IDD_DIALOG13                    113
#define    IDD_DIALOG14                    114
#define    IDD_DIALOG15                    115
#define    IDD_DIALOG16                    116
#define    IDD_DIALOG17                    117

#define	IDR_MAINFRAME					10
#define	IDR_DEBUG						11
#define    IDR_MEMORY                      12
#define	IDR_BREAKPOINT					13
#define	IDR_MESSAGELIST					14
#define	IDR_ASSEMBLER					15
#define	IDR_PROCEDURE					16
#define    IDR_BRKMEMORY                   17
#define    IDR_CHEAT                       18

#define    ID_SRAM_START                   0x200
#define    ID_SRAM_SST64K                  ID_SRAM_START
#define    ID_SRAM_SST128K                 ID_SRAM_START + 1
#define    ID_SRAM_ATMEL64K                ID_SRAM_START + 2
#define    ID_SRAM_MACRO64K                ID_SRAM_START + 3
#define    ID_SRAM_MACRO128K               ID_SRAM_START + 4
#define    ID_SRAM_PANASONIC64K            ID_SRAM_START + 5
#define    ID_SDRAM_SANYO128K              ID_SRAM_START + 6
#define    ID_SRAM_END                     ID_SDRAM_SANYO128K
#define    ID_EEPROM_START                 ID_SRAM_END
#define    ID_EEPROM_32K                   ID_EEPROM_START + 1
#define    ID_EEPROM_128K                  ID_EEPROM_START + 2
#define    ID_EEPROM_END                   ID_EEPROM_START + 2

#define    ID_SKIP_CPUSTART                0x45E
#define	ID_SKIP_CPU25					ID_SKIP_CPUSTART
#define    ID_SKIP_CPU50                   ID_SKIP_CPUSTART + 1
#define    ID_SKIP_CPU75                   ID_SKIP_CPUSTART + 2
#define    ID_SKIP_CPUEND                  ID_SKIP_CPU75

#define    ID_VIDEO_PLUG_START             0x47A
#define    ID_FILTER_BLUR                  ID_VIDEO_PLUG_START
#define    ID_FILTER_BILINEAR              ID_VIDEO_PLUG_START + 1
#define    ID_FILTER_MBLUR                 ID_VIDEO_PLUG_START + 2
#define    ID_FILTER_TVMODE                ID_VIDEO_PLUG_START + 3
#define    ID_FILTER_TRILINEAR             ID_VIDEO_PLUG_START + 4
#define    ID_FILTER_TFT                   ID_VIDEO_PLUG_START + 5
#define    ID_VIDEO_PLUG_END               0x49F

#define	ID_FILE_RECENT					0x1000

#define    ID_FILE_STATE_START             0x1030
#define    ID_FILE_STATE_END               0x104F

#define    ID_SOUND_PLUG_START             0x1050
#define    ID_SOUND_ECHO                   ID_SOUND_PLUG_START
#define    ID_SOUND_SUPERBASS              ID_SOUND_PLUG_START + 1
#define    ID_SOUND_PLUG_END               0x1120

#define    ID_SIO_PLUG_START               ID_SOUND_PLUG_END + 1
#define    ID_SIO_PLUG_END                 ID_SIO_PLUG_START + 1

#define  	ID_BKP_PLUG_START				0x1130
#define 	ID_BKP_PLUG_END					ID_BKP_PLUG_START + 1

#define	ID_APP_EXIT						0x400
#define    ID_APP_BIOS						0x401
#define	ID_APP_LOAD						0x402
#define	ID_EMU_START					0x403
#define	ID_EMU_RESET					0x404
#define	ID_HELP_ABOUT					0x405
#define	ID_SKIP_1FRAME					0x406
#define	ID_SKIP_2FRAME					0x407
#define	ID_SKIP_0FRAME					0x408
#define	ID_DEBUG_DIS					0x409
#define	ID_DEBUG_DIS1					0x40A
#define	ID_DEBUG_DIS2					0x40B
#define	ID_DEBUG_DIS3					0x40C
#define	ID_DEBUG_DIS4					0x40D
#define    ID_DEBUG_RESTART				0x40E
#define    ID_DEBUG_STOP   				0x40F
#define	ID_ZOOM_1						0x410
#define	ID_ZOOM_2						0x411
#define	ID_ZOOM_3						0x412
#define	ID_EMU_PAUSE					0x413
#define	ID_DEBUG_MESSAGE				0x414
#define	ID_DEBUG_IRQE					0x415
#define	ID_DEBUG_IRQX					0x416
#define	ID_MESSAGE_IRQ					0x417
#define	ID_MESSAGE_DMA					0x418
#define	ID_MESSAGE_SWI					0x419
#define	ID_MESSAGE_SWI5					0x41A
#define	ID_MESSAGE_POWER				0x41B
#define	ID_MESSAGE_TIMER				0x41C
#define	ID_DEBUG_GO					    0x41D
#define	ID_DEBUG_SAVE					0x41E
#define	ID_DEBUG_LOAD					0x41F
#define	ID_MESSAGE_CPU					0x420
#define	ID_DEBUG_REDRAW					0x421
#define	ID_DEBUG_BREAKPOINT				0x422
#define	ID_BREAKPOINT_DEL				0x423
#define	ID_BREAKPOINT_DELALL			0x424
#define	ID_BREAKPOINT_DISABLE			0x425
#define	ID_BREAKPOINT_DISABLEALL		0x426
#define	ID_DEBUG_PAUSE					0x427
#define	ID_DEBUG_PALBG					0x428
#define	ID_DEBUG_PALOBJ					0x429
#define	ID_DEBUG_DMA					0x42A
#define	ID_MESSAGE_CLEARLIST			0x42B
#define	ID_DEBUG_START_DMA				0x42C
#define	ID_DEBUG_IRQE1					0x42D
#define	ID_DEBUG_IRQE2					0x42E
#define	ID_DEBUG_IRQE3					0x42F
#define	ID_DEBUG_IRQE4					0x430
#define	ID_DEBUG_IRQE5					0x431
#define	ID_DEBUG_IRQE6					0x432
#define	ID_DEBUG_IRQE7					0x433
#define	ID_DEBUG_IRQE8					0x434
#define	ID_DEBUG_IRQE9					0x435
#define	ID_DEBUG_IRQE10					0x436
#define	ID_DEBUG_IRQE11					0x437
#define	ID_DEBUG_IRQE12					0x438
#define	ID_DEBUG_IRQE13					0x439
#define	ID_DEBUG_IRQE14					0x43A
#define	ID_DEBUG_IRQX1					0x43B
#define	ID_DEBUG_IRQX2					0x43C
#define	ID_DEBUG_IRQX3					0x43D
#define	ID_DEBUG_IRQX4					0x43E
#define	ID_DEBUG_IRQX5					0x43F
#define	ID_DEBUG_IRQX6					0x440
#define	ID_DEBUG_IRQX7					0x441
#define	ID_DEBUG_IRQX8					0x442
#define	ID_DEBUG_IRQX9					0x443
#define	ID_DEBUG_IRQX10					0x444
#define	ID_DEBUG_IRQX11					0x445
#define	ID_DEBUG_IRQX12					0x446
#define	ID_DEBUG_IRQX13					0x447
#define	ID_DEBUG_IRQX14					0x448
#define	ID_DEBUG_WRITEFILE_START		0x449
#define	ID_DEBUG_WRITEFILE_PAUSE		0x44A
#define	ID_DEBUG_WRITEFILE_RESUME		0x44B
#define	ID_DEBUG_WRITEFILE_STOP			0x44C
#define	ID_DEBUG_ENETER_SWI				0x44D
#define	ID_SOUND						0x44E
#define	ID_SOUND_FIFOA					0x44F
#define	ID_SOUND_FIFOB					0x450
#define	ID_BREAKPOINT_CONDITON			0x451
#define	ID_MESSAGE_TOBREAKPOINT			0x452
#define	ID_MESSAGE_SELECTALL			0x453
#define	ID_MESSAGE_DESELECTALL			0x454
#define	ID_MESSAGE_DELETESELECTED		0x455
#define    ID_MESSAGE_COPY                 0x45B
#define	ID_DEBUG_RECENT				    0x457
#define	ID_BREAKPOINT_COPY				ID_MESSAGE_COPY
#define	ID_ASSEMBLER_COPY				0x45C
#define	ID_ASSEMBLER_DELETEALL			0x45D

#define    ID_MEMORY_SAVE                  0x600
#define    ID_MEMORY_LOAD                  0x601
#define    ID_MESSAGE_GO                   0x602
#define    ID_MEMORY_RESET                 0x603
#define    ID_MESSAGE_EXCEPTIONS           0x604
#define    ID_MESSAGE_STACK                0x605
#define    ID_DEBUG_WRITEFILE_ALL          0x606
#define    ID_MEMORY_FILLZERO              0x607
#define    ID_MEMORY_SELECTALL             0x608
#define    ID_MEMORY_COPY                  0x609
#define    ID_MEMORY_PASTE                 0x60A

#define	ID_DEBUG_PROCEDURE				0x464
#define	ID_ASSEMBLER_PROCEDURE			0x465
#define	ID_PROCEDURE_NEW				0x466
#define	ID_PROCEDURE_DEL				0x467
#define	ID_PROCEDURE_EDIT				0x468
#define	ID_PROCEDURE_DELALL				0x469
#define	ID_PROCEDURE_MESSAGE			0x46A
#define	ID_FRAMEBUFFER_GDI				0x46B
#define	ID_FRAMEBUFFER_DDRAW			0x46C
#define    ID_FRAMEBUFFER_DDRAWFULLSCREEN  0x46d
#define	ID_SKIP_AUTO					0x46E
#define    ID_DEBUG_SPRITE					0x46F
#define    ID_SOUND_SAVE                   0x470
#define    ID_BREAKPOINT_GO                0x471
#define    ID_EMU_RESUME                   0x472
#define    ID_EMU_STOP                     0x473
#define	ID_PROCEDURE_GO					0x474
#define	ID_TEMP							0x475
#define    ID_DEBUG_BACKGROUND             0x476
#define    ID_BRIGHTNESS                   0x477
#define    ID_DEBUG_SOURCE                 0x478

#define    ID_SKIP_SYNCRO                  0x4A0
#define    ID_APP_USEBIOS                  0x4A1
#define    ID_SKIPBIOSINTRO                0x4A2
#define    ID_APP_RESETBIOS                0x4A3
#define	ID_MESSAGE_DMA0					0x4A4
#define	ID_MESSAGE_DMA1					0x4A5
#define	ID_MESSAGE_DMA2					0x4A6
#define	ID_MESSAGE_DMA3					0x4A7

#define    ID_LAYER_0                      0x4A8
#define    ID_LAYER_1                      0x4A9
#define    ID_LAYER_2                      0x4AA
#define    ID_LAYER_3                      0x4AB
#define    ID_LAYER_OBJ                    0x4AC
#define    ID_LAYER_WIN0                   0x4AD
#define    ID_LAYER_WIN1                   0x4AE
#define    ID_LAYER_WINOBJ                 0x4AF

#define    ID_DEBUG_BKMEMORY               0x4B0

#define	ID_SKIP_3FRAME					0x4B1
#define	ID_SKIP_4FRAME					0x4B2

#define    ID_SOUND_GBC1                   0x4B3
#define    ID_SOUND_GBC2                   0x4B4
#define    ID_SOUND_GBC3                   0x4B5
#define    ID_SOUND_GBC4                   0x4B6

#define    ID_EMU_AUTOSTART                0x4B7
#define    ID_FILE_RESET                   0x4B8
#define    ID_SOUND_ALL                    0x4B9
#define    ID_LAYER_ALL                    0x4BA
#define    ID_KEY_CONFIG                   0x4BB

#define    ID_MESSAGE_FIND                 0x4BC

#define    ID_GAMEPAK_AUTO                 0x4BD

#define    ID_BRKMEM_NEW                   0x4BE
#define    ID_BRKMEM_DELETE                0x4BF
#define    ID_BRKMEM_DELETEALL             0x4C0
#define    ID_BRKMEM_DISABLEDALL           0x4C1

#define    ID_SET_PROPERTY                 0x4C2

#define    ID_FILE_STATE_RESET             0x4C3
#define    ID_FILE_STATE_AUTOLOADMOSTRCNT  0x4C4
#define    ID_FILE_STATE_SAVE              0x4C5
#define    ID_FILE_STATE_RECORDGAME        0x4C6
#define    ID_FILE_STATE_REWINDGAME        0x4C7

#define    ID_FILE_CHEAT_AUTOLOAD          0x4C8
#define    ID_FILE_CHEAT_SAVE              0x4C9
#define    ID_FILE_CHEAT_LOAD              0x4CA
#define    ID_FILE_CHEAT_LIST              0x4CB
#define    ID_CHEAT_NEW_CODEBREAKER        0x4CD
#define    ID_CHEAT_NEW_GAMESHARK          0x4CE
#define    ID_CHEAT_DELETE                 0x4CF
#define    ID_CHEAT_DELETE_ALL             0x4D0
#define    ID_APP_IPSPATCH                 0x4D1
#define    ID_FILE_CHEAT_DISABLED          0x4D2

#define	IDM_STATUSBAR					0x5DF
#define	IDM_STATUSBARDEBUG				0x5E0
#define    IDM_TOOLBARDEBUG				0x5E1
#define    IDM_TOOLBAR                     IDM_TOOLBARDEBUG

#define    IDI_WINXP						1
#define	IDI_ICON1						2
#define	IDI_HANDPOINT					0x7700
#define	IDI_LOGO						0x7701
#define    IDI_TOOLBAR_DEBUG				0x7702
#define    IDI_PICKER						0x7703
#define    IDI_ICON2                       0x7704
#define	IDI_MAN							0x7705
#define    IDI_TOOLBARD_DEBUG              0x7706
#define    IDI_BUTTON_RIGHT                0x7707
#define    IDI_BUTTON_LEFT                 0x7708
#define    IDI_BUTTON_UP                   0x7709
#define    IDI_BUTTON_DOWN                 0x770A
#define    IDI_INFO_REG                    0x770B

#define	IDC_LIST1						0x1400
#define    IDC_CPU                         0x1401
#define    IDC_MEM                         0x1402
#define	IDC_LIST2						0x1401
#define	IDC_EDIT1						0x1402
#define	IDC_STATUS						0x1403
#define	IDC_IRQRET						0x1404
#define	IDC_CYCLE						0x1405
#define	IDC_MODE						0x1406
#define	IDC_EDIT2						0x1407
#define    IDC_LIST5                       0x1408
#define    IDC_LIST6                       0x1409
#define	IDC_COMBOBOX1					0x140B
#define	IDC_LIST3						0x140C
#define	IDC_CHK0I						0x140D
#define	IDC_CHK1I						0x140E
#define	IDC_CHK2I						0x140F
#define	IDC_CHK3I						0x1410
#define	IDC_VSBMEM						0x1411
#define	IDC_DEBUG_STATUSBAR				0x1412
#define	IDC_DMA0DAD						0x1413
#define	IDC_DMA0SAD						0x1414
#define	IDC_DMA0CNT						0x1415
#define	IDC_DMA1DAD						0x1416
#define	IDC_DMA1SAD						0x1417
#define	IDC_DMA2DAD						0x1418
#define	IDC_DMA2SAD						0x1419
#define	IDC_DMA3DAD						0x141A
#define	IDC_DMA3SAD						0x141B
#define	IDC_DMA0START					0x141C
#define	IDC_DMA1START					0x141D
#define	IDC_DMA2START					0x141E
#define	IDC_DMA3START					0x141F
#define	IDC_CHK0RP						0x1420
#define	IDC_CHK0RL						0x1421
#define	IDC_CHK1RP						0x1422
#define	IDC_CHK1RL						0x1423
#define	IDC_CHK2RP						0x1424
#define	IDC_CHK2RL						0x1425
#define	IDC_CHK3RP						0x1426
#define	IDC_CHK3RL						0x1427
#define	IDC_CHK0E						0x1428
#define	IDC_CHK1E						0x1429
#define	IDC_CHK2E						0x142A
#define	IDC_CHK3E						0x142B
#define	IDC_BREAKPOINTADRESS			0x142C
#define	IDC_LINK						0x142D
#define	IDC_LOGO						0x142E
#define	IDC_CHKMESSAGE					0x142F
#define	IDC_LIST4						0x1430
#define	IDC_COMBOBOX2					0x1431
#define	IDC_BUTTON1						0x1432
#define    IDC_TRACK2						0x1433
#define	IDC_COMBOBOX3					0x1436
#define	IDC_RASTERLINE					0x1437
#define    IDC_SPIN1						0x1438
#define	IDC_RADIO1						0x1439
#define	IDC_RADIO2						0x143A
#define	IDC_RADIO3						0x143B
#define	IDC_RADIO4						0x143C
#define	IDC_RADIO5						0x143D
#define	IDC_RADIO6						0x143E
#define    IDC_RADIO7                      0x143F
#define    IDC_RADIO8                      0x1440
#define    IDC_RADIO9                      0x1441
#define    IDC_RADIO10                     0x1442
#define    IDC_VSBDIS						0x1441
#define    IDC_TAB1						0x1442
#define    IDC_HSBSPR						0x1443
#define    IDC_VSBSPR						0x1444
#define    IDC_TRACK1						0x1445
#define    IDC_SPRITE						0x1446
#define    IDC_SPRITE_ENA					0x1447
#define    IDC_SPRITE_PRI					0x1448
#define    IDC_SPRITE_POS					0x1449
#define    IDC_SPRITE_COL					0x144A
#define    IDC_SPRITE_PAL					0x144B
#define    IDC_SPRITE_SIZ					0x144C
#define    IDC_SPRITE_ROT					0x144D
#define    IDC_SPRITE_ATTR0                0x144E
#define    IDC_SPRITE_ATTR1                0x144F
#define    IDC_SPRITE_ATTR2                0x1450
#define	IDC_BUTTON2						0x1451
#define    IDC_HSBBKG						0x1452
#define    IDC_VSBBKG						0x1453
#define    IDC_DRAWMODE                    0x1454
#define    IDC_TRKBRIGHTNESS               0x1455
#define    IDC_WMA                         0x1456

#define    IDC_BUTTON_A                    0x1457
#define    IDC_BUTTON_B                    0x1458
#define    IDC_BUTTON_SELECT               0x1459
#define    IDC_BUTTON_START                0x145A
#define    IDC_BUTTON_RIGHT                0x145B
#define    IDC_BUTTON_LEFT                 0x145C
#define    IDC_BUTTON_UP                   0x145D
#define    IDC_BUTTON_DOWN                 0x145E
#define    IDC_BUTTON_R                    0x145F
#define    IDC_BUTTON_L                    0x1460
#define    IDC_BUTTON3                     0x146D

#define    IDC_RELOAD                      0x1461

#define    IDC_RADIO11                     0x1462
#define    IDC_RADIO12                     0x1463
#define    IDC_RADIO13                     0x1464
#define    IDC_RADIO14                     0x1465

#define    IDC_CHECK1                      0x1466
#define    IDC_CHECK2                      0x1467
#define    IDC_CHECK3                      0x1468
#define    IDC_CHECK4                      0x1469
#define    IDC_CHECK5                      IDC_RADIO11
#define    IDC_CHECK6                      IDC_RADIO12

#define    IDC_UPDIOREG                    0x146A

#define    IDC_RADIO15                     0x146B
#define    IDC_RADIO16                     0x146C

#define ID_DLG0_FILE_TITLE                  0xC000
#define ID_DLG0_FILE_ROM                    0xC001
#define ID_DLG0_FILE_BIOS                   0xC006
#define ID_DLG0_FILE_STATE                  0xC00D
#define ID_DLG0_FILE_STATE_LOAD             0xC016
#define ID_DLG0_FILE_STATE_SAVE             0xC017
#define ID_DLG0_FILE_CHEAT                  0xC018
#define ID_DLG0_FILE_EMULATION              0xC01E
#define ID_DLG0_FILE_RCNFILES               0xC026
#define ID_DLG0_FILE_BACKUP                 0xC029

#define ID_DLG0_DEBUGGER_TITLE              0xC080

#define ID_DLG0_OPTIONS_TITLE               0xC100
#define ID_DLG0_OPTIONS_EMUSPEED            0xC101
#define ID_DLG0_OPTIONS_CPUSPEED            0xC108
#define ID_DLG0_OPTIONS_GAMEPAK             0xC10E
#define ID_DLG0_OPTIONS_FLASH64             0xC10F
#define ID_DLG0_OPTIONS_FLASH128            0xC114
#define ID_DLG0_OPTIONS_SIO                 0xC11C
#define ID_DLG0_OPTIONS_SO                  0xC11F
#define ID_DLG0_OPTIONS_AF                  0xC129
#define ID_DLG0_OPTIONS_ZOOM                0xC12C
#define ID_DLG0_OPTIONS_FB                  0xC130
#define ID_DLG0_OPTIONS_VF                  0xC134
#define ID_DLG0_OPTIONS_LAYERS              0xC135


#define ID_DLG0_HELP_TITLE                  0xC180

#define ID_DLG1_DEBUGGER_TITLE              0xC400
#define ID_DLG1_DEBUGGER_WRITE              0xC40A
#define ID_DLG1_DEBUGGER_RCNFILES           0xc413

#define ID_DLG1_VIEW_TITLE                  0xC480

#define ID_DLG2_FILE_TITLE                  0xC800

#define ID_DLG3_FILE_TITLE                  0xE000

