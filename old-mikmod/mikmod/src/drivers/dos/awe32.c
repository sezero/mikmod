/*

Name: AWE32.C

Description:
 Mikmod driver for output on AWE32 & SB32 (native mode i.e. using
 the onboard DRAM)

Portability:

 MSDOS:  BC(y)   Watcom(y)       DJGPP(y)
 Win95:  n
 Os2:    n
 Linux:  n

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <dos.h>
#include <conio.h>
#include "mikmod.h"
#include "mirq.h"
#include "sb.h"

#define AWED0 0x000
#define AWED1 0x400
#define AWED2 0x402
#define AWED3 0x800
#define AWED4 0x802
#define AWED0S (AWED0<<16)
#define AWED1S (AWED1<<16)
#define AWED2S (AWED2<<16)
#define AWED3S (AWED3<<16)
#define AWED4S (AWED4<<16)

#define CPF AWED0S
#define PTRX AWED0S|0x20
#define CVCF AWED0S|0x40
#define VTFT AWED0S|0x60
#define PSST AWED0S|0xC0
#define CSL AWED0S|0xE0
#define CCCA AWED1S
#define HWCF4 AWED1S|0x20|9
#define HWCF5 AWED1S|0x20|10
#define HWCF6 AWED1S|0x20|13
#define SMALR AWED1S|0x20|20
#define SMARR AWED1S|0x20|21
#define SMALW AWED1S|0x20|22
#define SMARW AWED1S|0x20|23
#define SMLD AWED1S|0x20|26
#define SMRD AWED2S|0x20|26
#define WC AWED2S|0x20|27
#define HWCF1 AWED1S|0x20|29
#define HWCF2 AWED1S|0x20|30
#define HWCF3 AWED1S|0x20|31
#define INIT1 AWED1S|0x40
#define INIT2 AWED2S|0x40
#define INIT3 AWED1S|0x60
#define INIT4 AWED2S|0x60
#define ENVVOL AWED1S|0x80
#define DCYSUSV AWED1S|0xA0
#define ENVVAL AWED1S|0xC0
#define DCYSUS AWED1S|0xE0
#define ATKHLDV AWED2S|0x80
#define LFO1VAL AWED2S|0xA0
#define ATKHLD AWED2S|0xC0
#define LFO2VAL AWED2S|0xE0
#define IP AWED3S
#define IFATN AWED3S|0x20
#define PEFE AWED3S|0x40
#define FMMOD AWED3S|0x60
#define TREMFRQ AWED3S|0x80
#define FM2FRQ2 AWED3S|0xA0

// additional (undocumented?) registers
#define DETECT1 AWED3S|0xE0
#define DETECT2 AWED1S|0x3D
#define DETECT3 AWED1S|0x3E


int DRAMSize;
int AWE_BPM;

UWORD InitData[] =
{  0x3FF,0x30,0x7FF,0x130,0xBFF,0x230,0xFFF,0x330,
   0x13FF,0x430,0x17FF,0x530,0x1BFF,0x630,0x1FFF,0x730,
   0x23FF,0x830,0x27FF,0x930,0x2BFF,0xA30,0x2FFF,0xB30,
   0x33FF,0xC30,0x37FF,0xD30,0x3BFF,0xE30,0x3FFF,0xF30,
   0x43FF,0x030,0x47FF,0x130,0x4BFF,0x230,0x4FFF,0x330,
   0x53FF,0x430,0x57FF,0x530,0x5BFF,0x630,0x5FFF,0x730,
   0x63FF,0x830,0x67FF,0x930,0x6BFF,0xA30,0x6FFF,0xB30,
   0x73FF,0xC30,0x77FF,0xD30,0x7BFF,0xE30,0x7FFF,0xF30,

   0x83FF,0x030,0x87FF,0x130,0x8BFF,0x230,0x8FFF,0x330,
   0x93FF,0x430,0x97FF,0x530,0x9BFF,0x630,0x9FFF,0x730,
   0xA3FF,0x830,0xA7FF,0x930,0xABFF,0xA30,0xAFFF,0xB30,
   0xB3FF,0xC30,0xB7FF,0xD30,0xBBFF,0xE30,0xBFFF,0xF30,
   0xC3FF,0x030,0xC7FF,0x130,0xCBFF,0x230,0xCFFF,0x330,
   0xD3FF,0x430,0xD7FF,0x530,0xDBFF,0x630,0xDFFF,0x730,
   0xE3FF,0x830,0xE7FF,0x930,0xEBFF,0xA30,0xEFFF,0xB30,
   0xF3FF,0xC30,0xF7FF,0xD30,0xFBFF,0xE30,0xFFFF,0xF30
};

UWORD InitData2[] =
{  0xC10,0x8470,0x14FE,0xB488,0x167F,0xA470,0x18E7,0x84B5,
   0x1B6E,0x842A,0x1F1D,0x852A,0xDA3,0xF7C,0x167E,0x7254,
   0x0000,0x842A,1,0x852A,0x18E6,0xBAA,0x1B6D,0x7234,
   0x229F,0x8429,0x2746,0x8529,0x1F1C,0x6E7,0x229E,0x7224,
   0xDA4,0x8429,0x2C29,0x8529,0x2745,0x7F6,0x2C28,0x7254,
   0x383B,0x8428,0x320F,0x8528,0x320E,0xF02,0x1341,0x7264,
   0x3EB6,0x8428,0x3EB9,0x8528,0x383A,0xFA9,0x3EB5,0x7294,
   0x3EB7,0x8474,0x3EBA,0x8575,0x3EB8,0x44C3,0x3EBB,0x45C3,

   0x0000,0xA404,1,0xA504,0x141F,0x671,0x14FD,0x287,
   0x3EBC,0xE610,0x3EC8,0xC7B,0x31A,0x7E6,0x3EC8,0x86F7,
   0x3EC0,0x821E,0x3EBE,0xD280,0x3EBD,0x21F,0x3ECA,0x386,
   0x3EC1,0xC03,0x3EC9,0x31E,0x3ECA,0x8C4C,0x3EBF,0xC55,
   0x3EC9,0xC280,0x3EC4,0xBC84,0x3EC8,0xEAD,0x3EC8,0xD380,
   0x3EC2,0x8F7E,0x3ECB,0x219,0x3ECB,0xD2E6,0x3EC5,0x31F,
   0x3EC6,0xC380,0x3EC3,0x327F,0x3EC9,0x265,0x3EC9,0x8319,
   0x1342,0xD3E6,0x3EC7,0x337F,0x0000,0x8365,0x1420,0x9570
};


UBYTE VolumeTable[257] =
{  127, 111, 102, 95, 90, 86, 83, 80, 77, 75, 72, 71, 69, 67, 65, 64,
   63, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 51, 50, 49, 48,
   48, 47, 46, 46, 45, 45, 44, 43, 43, 42, 42, 41, 41, 40, 40, 39,
   39, 38, 38, 37, 37, 37, 36, 36, 35, 35, 35, 34, 34, 34, 33, 33,
   32, 32, 32, 31, 31, 31, 30, 30, 30, 30, 29, 29, 29, 28, 28, 28,
   28, 27, 27, 27, 26, 26, 26, 26, 25, 25, 25, 25, 24, 24, 24, 24,
   23, 23, 23, 23, 23, 22, 22, 22, 22, 21, 21, 21, 21, 21, 20, 20,
   20, 20, 20, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 17, 17,
   17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
   14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12,
   12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10,
   10, 10, 10, 10, 9, 9, 9, 9, 9, 9, 9, 9, 8, 8, 8, 8,
   8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5,
   4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3,
   3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   2
};


/*
  Frequency Table Math calulation --

    for (k=0; k<FrqMax; k++)
    {  f = (int)(log((float)(k<<4))*5909.27-5850.98);
       if(f>0xFFFF) f = 0xFFFF;
       FrqTable[k] = (short)f;
    }
*/

UWORD LowFrqTable[512] =
{  6144, 10532, 14628, 17024, 18724, 20043, 21120, 22031, 22820, 23516, 24139, 24702, 25216, 25689, 26127, 26535,
   26916, 27275, 27612, 27932, 28235, 28523, 28798, 29061, 29312, 29554, 29785, 30008, 30223, 30431, 30631, 30825,
   31012, 31194, 31371, 31542, 31708, 31870, 32028, 32181, 32331, 32477, 32619, 32758, 32894, 33027, 33157, 33284,
   33408, 33530, 33650, 33767, 33881, 33994, 34104, 34213, 34319, 34424, 34527, 34628, 34727, 34825, 34921, 35015,
   35108, 35200, 35290, 35379, 35467, 35553, 35638, 35722, 35804, 35886, 35966, 36046, 36124, 36201, 36277, 36353,
   36427, 36500, 36573, 36645, 36715, 36785, 36854, 36923, 36990, 37057, 37123, 37188, 37253, 37317, 37380, 37443,
   37504, 37566, 37626, 37686, 37746, 37804, 37863, 37920, 37977, 38034, 38090, 38146, 38200, 38255, 38309, 38362,
   38415, 38468, 38520, 38572, 38623, 38673, 38724, 38774, 38823, 38872, 38921, 38969, 39017, 39064, 39111, 39158,
   39204, 39250, 39296, 39341, 39386, 39431, 39475, 39519, 39563, 39606, 39649, 39692, 39734, 39776, 39818, 39859,
   39900, 39941, 39982, 40022, 40062, 40102, 40142, 40181, 40220, 40259, 40297, 40335, 40373, 40411, 40449, 40486,
   40523, 40560, 40596, 40633, 40669, 40705, 40741, 40776, 40811, 40846, 40881, 40916, 40950, 40985, 41019, 41053,
   41086, 41120, 41153, 41186, 41219, 41252, 41284, 41317, 41349, 41381, 41413, 41445, 41476, 41507, 41539, 41570,
   41600, 41631, 41662, 41692, 41722, 41752, 41782, 41812, 41842, 41871, 41900, 41930, 41959, 41988, 42016, 42045,
   42073, 42102, 42130, 42158, 42186, 42214, 42241, 42269, 42296, 42324, 42351, 42378, 42405, 42432, 42458, 42485,
   42511, 42538, 42564, 42590, 42616, 42642, 42668, 42693, 42719, 42744, 42769, 42795, 42820, 42845, 42870, 42894,
   42919, 42944, 42968, 42992, 43017, 43041, 43065, 43089, 43113, 43137, 43160, 43184, 43207, 43231, 43254, 43277,
   43300, 43323, 43346, 43369, 43392, 43415, 43437, 43460, 43482, 43505, 43527, 43549, 43571, 43593, 43615, 43637,
   43659, 43680, 43702, 43724, 43745, 43766, 43788, 43809, 43830, 43851, 43872, 43893, 43914, 43935, 43955, 43976,
   43996, 44017, 44037, 44058, 44078, 44098, 44118, 44138, 44158, 44178, 44198, 44218, 44238, 44257, 44277, 44296,
   44316, 44335, 44355, 44374, 44393, 44412, 44431, 44450, 44469, 44488, 44507, 44526, 44545, 44563, 44582, 44601,
   44619, 44637, 44656, 44674, 44692, 44711, 44729, 44747, 44765, 44783, 44801, 44819, 44837, 44854, 44872, 44890,
   44907, 44925, 44942, 44960, 44977, 44995, 45012, 45029, 45046, 45064, 45081, 45098, 45115, 45132, 45149, 45165,
   45182, 45199, 45216, 45232, 45249, 45266, 45282, 45299, 45315, 45331, 45348, 45364, 45380, 45397, 45413, 45429,
   45445, 45461, 45477, 45493, 45509, 45525, 45541, 45556, 45572, 45588, 45603, 45619, 45635, 45650, 45666, 45681,
   45696, 45712, 45727, 45742, 45758, 45773, 45788, 45803, 45818, 45833, 45848, 45863, 45878, 45893, 45908, 45923,
   45938, 45952, 45967, 45982, 45996, 46011, 46026, 46040, 46055, 46069, 46084, 46098, 46112, 46127, 46141, 46155,
   46169, 46184, 46198, 46212, 46226, 46240, 46254, 46268, 46282, 46296, 46310, 46324, 46337, 46351, 46365, 46379,
   46392, 46406, 46420, 46433, 46447, 46460, 46474, 46487, 46501, 46514, 46528, 46541, 46554, 46568, 46581, 46594,
   46607, 46621, 46634, 46647, 46660, 46673, 46686, 46699, 46712, 46725, 46738, 46751, 46764, 46776, 46789, 46802,
   46815, 46827, 46840, 46853, 46865, 46878, 46891, 46903, 46916, 46928, 46941, 46953, 46966, 46978, 46990, 47003,
   47015, 47027, 47040, 47052, 47064, 47076, 47088, 47101, 47113, 47125, 47137, 47149, 47161, 47173, 47185, 47197,
   47209, 47221, 47233, 47244, 47256, 47268, 47280, 47292, 47303, 47315, 47327, 47338, 47350, 47362, 47373, 47385
};

UWORD HighFrqTable[1408 - (5*16)] =
{  47396, 47488, 47578, 47667, 47755, 47841, 47926, 48010, 48092, 48174, 48254, 48334, 48412, 48489, 48565, 48641,
   48715, 48788, 48861, 48933, 49003, 49073, 49142, 49211, 49278, 49345, 49411, 49476, 49541, 49605, 49668, 49731,
   49792, 49854, 49914, 49974, 50034, 50092, 50151, 50208, 50265, 50322, 50378, 50433, 50488, 50543, 50597, 50650,
   50703, 50756, 50808, 50860, 50911, 50961, 51012, 51062, 51111, 51160, 51209, 51257, 51305, 51352, 51399, 51446,
   51492, 51538, 51584, 51629, 51674, 51719, 51763, 51807, 51851, 51894, 51937, 51980, 52022, 52064, 52106, 52147,
   52188, 52229, 52270, 52310, 52350, 52390, 52430, 52469, 52508, 52547, 52585, 52623, 52661, 52699, 52737, 52774,
   52811, 52848, 52884, 52921, 52957, 52993, 53029, 53064, 53099, 53134, 53169, 53204, 53238, 53273, 53307, 53341,
   53374, 53408, 53441, 53474, 53507, 53540, 53572, 53605, 53637, 53669, 53701, 53733, 53764, 53795, 53827, 53858,
   53888, 53919, 53950, 53980, 54010, 54040, 54070, 54100, 54130, 54159, 54188, 54218, 54247, 54276, 54304, 54333,
   54361, 54390, 54418, 54446, 54474, 54502, 54529, 54557, 54584, 54612, 54639, 54666, 54693, 54720, 54746, 54773,
   54799, 54826, 54852, 54878, 54904, 54930, 54956, 54981, 55007, 55032, 55057, 55083, 55108, 55133, 55158, 55182,
   55207, 55232, 55256, 55280, 55305, 55329, 55353, 55377, 55401, 55425, 55448, 55472, 55495, 55519, 55542, 55565,
   55588, 55611, 55634, 55657, 55680, 55703, 55725, 55748, 55770, 55793, 55815, 55837, 55859, 55881, 55903, 55925,
   55947, 55968, 55990, 56011, 56033, 56054, 56076, 56097, 56118, 56139, 56160, 56181, 56202, 56223, 56243, 56264,
   56284, 56305, 56325, 56346, 56366, 56386, 56406, 56426, 56446, 56466, 56486, 56506, 56526, 56545, 56565, 56584,
   56604, 56623, 56643, 56662, 56681, 56700, 56719, 56738, 56757, 56776, 56795, 56814, 56833, 56851, 56870, 56889,
   56907, 56925, 56944, 56962, 56980, 56999, 57017, 57035, 57053, 57071, 57089, 57107, 57125, 57142, 57160, 57178,
   57195, 57213, 57230, 57248, 57265, 57283, 57300, 57317, 57334, 57352, 57369, 57386, 57403, 57420, 57437, 57453,
   57470, 57487, 57504, 57520, 57537, 57554, 57570, 57587, 57603, 57619, 57636, 57652, 57668, 57685, 57701, 57717,
   57733, 57749, 57765, 57781, 57797, 57813, 57829, 57844, 57860, 57876, 57891, 57907, 57923, 57938, 57954, 57969,
   57984, 58000, 58015, 58030, 58046, 58061, 58076, 58091, 58106, 58121, 58136, 58151, 58166, 58181, 58196, 58211,
   58226, 58240, 58255, 58270, 58284, 58299, 58314, 58328, 58343, 58357, 58372, 58386, 58400, 58415, 58429, 58443,
   58457, 58472, 58486, 58500, 58514, 58528, 58542, 58556, 58570, 58584, 58598, 58612, 58625, 58639, 58653, 58667,
   58680, 58694, 58708, 58721, 58735, 58748, 58762, 58775, 58789, 58802, 58816, 58829, 58842, 58856, 58869, 58882,
   58895, 58909, 58922, 58935, 58948, 58961, 58974, 58987, 59000, 59013, 59026, 59039, 59052, 59064, 59077, 59090,
   59103, 59115, 59128, 59141, 59153, 59166, 59179, 59191, 59204, 59216, 59229, 59241, 59254, 59266, 59278, 59291,
   59303, 59315, 59328, 59340, 59352, 59364, 59376, 59389, 59401, 59413, 59425, 59437, 59449, 59461, 59473, 59485,
   59497, 59509, 59521, 59532, 59544, 59556, 59568, 59580, 59591, 59603, 59615, 59626, 59638, 59650, 59661, 59673,
   59684, 59696, 59707, 59719, 59730, 59742, 59753, 59765, 59776, 59787, 59799, 59810, 59821, 59833, 59844, 59855,
   59866, 59877, 59889, 59900, 59911, 59922, 59933, 59944, 59955, 59966, 59977, 59988, 59999, 60010, 60021, 60032,
   60043, 60054, 60064, 60075, 60086, 60097, 60107, 60118, 60129, 60140, 60150, 60161, 60172, 60182, 60193, 60203,
   60214, 60225, 60235, 60246, 60256, 60266, 60277, 60287, 60298, 60308, 60319, 60329, 60339, 60350, 60360, 60370,
   60380, 60391, 60401, 60411, 60421, 60432, 60442, 60452, 60462, 60472, 60482, 60492, 60502, 60512, 60522, 60532,
   60542, 60552, 60562, 60572, 60582, 60592, 60602, 60612, 60622, 60632, 60641, 60651, 60661, 60671, 60680, 60690,
   60700, 60710, 60719, 60729, 60739, 60748, 60758, 60768, 60777, 60787, 60796, 60806, 60815, 60825, 60834, 60844,
   60853, 60863, 60872, 60882, 60891, 60901, 60910, 60919, 60929, 60938, 60947, 60957, 60966, 60975, 60985, 60994,
   61003, 61012, 61021, 61031, 61040, 61049, 61058, 61067, 61076, 61086, 61095, 61104, 61113, 61122, 61131, 61140,
   61149, 61158, 61167, 61176, 61185, 61194, 61203, 61212, 61221, 61229, 61238, 61247, 61256, 61265, 61274, 61283,
   61291, 61300, 61309, 61318, 61326, 61335, 61344, 61353, 61361, 61370, 61379, 61387, 61396, 61405, 61413, 61422,
   61430, 61439, 61448, 61456, 61465, 61473, 61482, 61490, 61499, 61507, 61516, 61524, 61533, 61541, 61549, 61558,
   61566, 61575, 61583, 61591, 61600, 61608, 61616, 61625, 61633, 61641, 61650, 61658, 61666, 61674, 61683, 61691,
   61699, 61707, 61715, 61724, 61732, 61740, 61748, 61756, 61764, 61772, 61781, 61789, 61797, 61805, 61813, 61821,
   61829, 61837, 61845, 61853, 61861, 61869, 61877, 61885, 61893, 61901, 61909, 61917, 61924, 61932, 61940, 61948,
   61956, 61964, 61972, 61980, 61987, 61995, 62003, 62011, 62019, 62026, 62034, 62042, 62050, 62057, 62065, 62073,
   62080, 62088, 62096, 62103, 62111, 62119, 62126, 62134, 62142, 62149, 62157, 62164, 62172, 62180, 62187, 62195,
   62202, 62210, 62217, 62225, 62232, 62240, 62247, 62255, 62262, 62270, 62277, 62285, 62292, 62299, 62307, 62314,
   62322, 62329, 62336, 62344, 62351, 62358, 62366, 62373, 62380, 62388, 62395, 62402, 62410, 62417, 62424, 62431,
   62439, 62446, 62453, 62460, 62468, 62475, 62482, 62489, 62496, 62503, 62511, 62518, 62525, 62532, 62539, 62546,
   62553, 62561, 62568, 62575, 62582, 62589, 62596, 62603, 62610, 62617, 62624, 62631, 62638, 62645, 62652, 62659,
   62666, 62673, 62680, 62687, 62694, 62701, 62708, 62715, 62721, 62728, 62735, 62742, 62749, 62756, 62763, 62770,
   62776, 62783, 62790, 62797, 62804, 62811, 62817, 62824, 62831, 62838, 62844, 62851, 62858, 62865, 62871, 62878,
   62885, 62892, 62898, 62905, 62912, 62918, 62925, 62932, 62938, 62945, 62952, 62958, 62965, 62972, 62978, 62985,
   62991, 62998, 63005, 63011, 63018, 63024, 63031, 63037, 63044, 63050, 63057, 63063, 63070, 63076, 63083, 63089,
   63096, 63102, 63109, 63115, 63122, 63128, 63135, 63141, 63148, 63154, 63160, 63167, 63173, 63180, 63186, 63192,
   63199, 63205, 63211, 63218, 63224, 63230, 63237, 63243, 63249, 63256, 63262, 63268, 63275, 63281, 63287, 63293,
   63300, 63306, 63312, 63318, 63325, 63331, 63337, 63343, 63350, 63356, 63362, 63368, 63374, 63381, 63387, 63393,
   63399, 63405, 63411, 63417, 63424, 63430, 63436, 63442, 63448, 63454, 63460, 63466, 63472, 63479, 63485, 63491,
   63497, 63503, 63509, 63515, 63521, 63527, 63533, 63539, 63545, 63551, 63557, 63563, 63569, 63575, 63581, 63587,
   63593, 63599, 63605, 63611, 63617, 63623, 63628, 63634, 63640, 63646, 63652, 63658, 63664, 63670, 63676, 63681,
   63687, 63693, 63699, 63705, 63711, 63717, 63722, 63728, 63734, 63740, 63746, 63751, 63757, 63763, 63769, 63775,
   63780, 63786, 63792, 63798, 63803, 63809, 63815, 63821, 63826, 63832, 63838, 63844, 63849, 63855, 63861, 63866,
   63872, 63878, 63883, 63889, 63895, 63900, 63906, 63912, 63917, 63923, 63929, 63934, 63940, 63945, 63951, 63957,
   63962, 63968, 63973, 63979, 63985, 63990, 63996, 64001, 64007, 64012, 64018, 64023, 64029, 64035, 64040, 64046,
   64051, 64057, 64062, 64068, 64073, 64079, 64084, 64090, 64095, 64101, 64106, 64111, 64117, 64122, 64128, 64133,
   64139, 64144, 64150, 64155, 64160, 64166, 64171, 64177, 64182, 64187, 64193, 64198, 64203, 64209, 64214, 64220,
   64225, 64230, 64236, 64241, 64246, 64252, 64257, 64262, 64268, 64273, 64278, 64284, 64289, 64294, 64299, 64305,
   64310, 64315, 64321, 64326, 64331, 64336, 64342, 64347, 64352, 64357, 64362, 64368, 64373, 64378, 64383, 64389,
   64394, 64399, 64404, 64409, 64415, 64420, 64425, 64430, 64435, 64440, 64446, 64451, 64456, 64461, 64466, 64471,
   64476, 64482, 64487, 64492, 64497, 64502, 64507, 64512, 64517, 64522, 64528, 64533, 64538, 64543, 64548, 64553,
   64558, 64563, 64568, 64573, 64578, 64583, 64588, 64593, 64598, 64603, 64608, 64613, 64618, 64623, 64628, 64633,
   64638, 64643, 64648, 64653, 64658, 64663, 64668, 64673, 64678, 64683, 64688, 64693, 64698, 64703, 64708, 64713,
   64718, 64723, 64727, 64732, 64737, 64742, 64747, 64752, 64757, 64762, 64767, 64772, 64776, 64781, 64786, 64791,
   64796, 64801, 64806, 64810, 64815, 64820, 64825, 64830, 64835, 64840, 64844, 64849, 64854, 64859, 64864, 64868,
   64873, 64878, 64883, 64888, 64892, 64897, 64902, 64907, 64911, 64916, 64921, 64926, 64930, 64935, 64940, 64945,
   64949, 64954, 64959, 64964, 64968, 64973, 64978, 64982, 64987, 64992, 64997, 65001, 65006, 65011, 65015, 65020,
   65025, 65029, 65034, 65039, 65043, 65048, 65053, 65057, 65062, 65067, 65071, 65076, 65081, 65085, 65090, 65094,
   65099, 65104, 65108, 65113, 65117, 65122, 65127, 65131, 65136, 65140, 65145, 65150, 65154, 65159, 65163, 65168,
   65172, 65177, 65182, 65186, 65191, 65195, 65200, 65204, 65209, 65213, 65218, 65222, 65227, 65231, 65236, 65240,
   65245, 65249, 65254, 65258, 65263, 65267, 65272, 65276, 65281, 65285, 65290, 65294, 65299, 65303, 65308, 65312,
   65317, 65321, 65325, 65330, 65334, 65339, 65343, 65348, 65352, 65356, 65361, 65365, 65370, 65374, 65379, 65383,
   65387, 65392, 65396, 65401, 65405, 65409, 65414, 65418, 65422, 65427, 65431, 65436, 65440, 65444, 65449, 65453,
   65457, 65462, 65466, 65470, 65475, 65479, 65483, 65488, 65492, 65496, 65501, 65505, 65509, 65513, 65518, 65522,
   65526, 65531, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535
};

unsigned int InitCommand[] = { INIT1, INIT2, INIT3, INIT4 };

typedef struct
{   SLONG  base,
           size,
           loopstart,
           loopend;
    SWORD  *samp;
    int    flags;
} TAWEsamp;

static TAWEsamp *AWEsamp;
static int AWEsampNo = 0;

static int awe_voices;

#define FrqMax (1408 - (5*16))

typedef struct
{   UBYTE kick;                  // =1 -> sample has to be restarted
    UBYTE active;                // =1 -> sample is playing
    UWORD flags;                 // 16/8 bits looping/one-shot
    short handle;                // identifies the sample
    long  start;                 // start index
    long  size;                  // samplesize
    long  reppos;                // loop start
    long  repend;                // loop end
    long  frq;                   // current frequency
    short vol;                   // current volume
    short pan;                   // current panning position
    int   panold;
} GHOLD;

static GHOLD *ghld = NULL;

static UWORD sb_silenceLen;
static SWORD sb_silenceLenAdd = 0;
static UWORD awe_timerReq;

#define sb_TC 210
#define sb_Freq (1000000L / (256-sb_TC))


//  Define some important SB i/o ports:

#define MIXER_ADDRESS           (sb_port + 0x4)
#define MIXER_DATA              (sb_port + 0x5)
#define DSP_RESET               (sb_port + 0x6)
#define DSP_READ_DATA           (sb_port + 0xa)
#define DSP_WRITE_DATA          (sb_port + 0xc)
#define DSP_WRITE_STATUS        (sb_port + 0xc)
#define DSP_DATA_AVAIL          (sb_port + 0xe)


static void SB_SetBPM(int bpm)
{
    sb_silenceLen = sb_Freq*125 / (bpm*50) - 1;
    awe_timerReq  = 44100*125 / (bpm*50);
}


static void SB_StartSilence()
{
    SB_WriteDSP(0x80);
    SB_WriteDSP((sb_silenceLen+sb_silenceLenAdd) & 0xFF);
    SB_WriteDSP((sb_silenceLen+sb_silenceLenAdd) >> 8);
}


static void AWE32WW(ULONG index, UWORD Data)
{
    _disable();
    outpw(AWE32Base + 0x802, (index) & 0xffff);
    outpw(AWE32Base + ((index) >> 16), Data);
    _enable();
}


static void AWE32WD(ULONG index, ULONG Data)
{
    _disable();
    outpw(AWE32Base + 0x802,index & 0xffff);
    outpw(AWE32Base + (index >> 16), Data);
    outpw(AWE32Base + (index >> 16) + 2, Data >> 16);
    _enable();
}


static UWORD AWE32RW(ULONG index)
{
    UWORD k;

    _disable();
    outpw(AWE32Base+0x802,index&0xffff);
    k = inpw(AWE32Base+(index>>16));
    _enable();
    return k;
}


static ULONG AWE32RD(ULONG index)
{
    ULONG k;

    _disable();
    outpw(AWE32Base+0x802,index&0xffff);
    k  = inpw(AWE32Base+(index>>16));
    k |= inpw(AWE32Base+(index>>16)+2)<<16;
    _enable();
    return k;
}


static int AWE32Detect()             // detect AWE32
{
    if((AWE32RW(DETECT1) & 0xC)  != 0xC) return 0;
    if((AWE32RW(DETECT2) & 0x58) != 0x58) return 0;
    if((AWE32RW(DETECT3) & 3)    != 3) return 0;
    return 1;
}


static void AWE32InitReg(UWORD *Data)
{
    int k,i;

    for(k=0; k<4; ++k)
        for(i=0; i<32; ++i)
            AWE32WW(InitCommand[k]+i,Data[k*32+i]);
}


static void AWE32InitReg2(UWORD *Data)
{
    int k,i;
    
    for(k=0; k<4; ++k)
      for(i=0; i<32; ++i)
      {   unsigned int TempData = Data[k*32+i];
          AWE32WW(InitCommand[k]+i,TempData | (i&1)<<15);
      }
}


static int aweTimerGet()
{
    return AWE32RW(WC);
}

static long aweTimerDiff(int t1, int t2)
{
    if (t1 < t2) t1 += 0x10000;
    return t1-t2;
}


/*********************************************/
/* Wait Delay number of AWE32 44100Hz clocks */
/*********************************************/
static void AWE32Wait(long Clocks)
{
    int k;

    k = aweTimerGet(); // 44100Hz clock
    while(aweTimerDiff(aweTimerGet(),k) < Clocks);
}


/****************************************/
/* Initialization of the AWE32 hardware */
/****************************************/
static void AWE32Init(void)
{
   int k;

   AWE32WW(HWCF3,0); // turn off audio?

   AWE32WW(HWCF1,0x59);  // HWCF1
   AWE32WW(HWCF2,0x20);  // HWCF2

   for(k=0; k<32; ++k)
   {   // init envelope engine
       AWE32WW(DCYSUSV,0x80);   // DCYSUSV
   }

   for(k=0; k<32; ++k)
   {   AWE32WW(ENVVOL+k,0);
       AWE32WW(ENVVAL+k,0);
       AWE32WW(DCYSUS+k,0);
       AWE32WW(ATKHLDV+k,0);
       AWE32WW(LFO1VAL+k,0);
       AWE32WW(ATKHLD,0);
       AWE32WW(LFO2VAL,0);

       AWE32WW(IP+k,0);
       AWE32WW(IFATN+k,0);
       AWE32WW(PEFE+k,0);
       AWE32WW(FMMOD+k,0);
       AWE32WW(TREMFRQ+k,0);
       AWE32WW(FM2FRQ2+k,0);
       AWE32WD(PTRX+k,0);
       AWE32WD(VTFT+k,0);
       AWE32WD(PSST+k,0);
       AWE32WD(CSL+k,0);
       AWE32WD(CCCA+k,0);
   }

   AWE32Wait(2);

   for(k=0; k<32; ++k)
   {   // init sound engine
       AWE32WW(CPF+k,0);       // CPF
       AWE32WW(CVCF+k,0);      // CVCF
   }

   AWE32WW(SMALR,0);    // init effects engine
   AWE32WW(SMARR,0);
   AWE32WW(SMALW,0);
   AWE32WW(SMARW,0);

   AWE32InitReg(InitData);
   AWE32Wait(0x400);
   AWE32InitReg2(InitData);
   AWE32InitReg2(InitData2);

   AWE32WD(HWCF4,0);
   AWE32WD(HWCF5,0x83);
   AWE32WD(HWCF6,0x8000);

   AWE32InitReg(InitData2);

   AWE32WW(HWCF3,4);

   AWE32WW(DCYSUSV+30,0x80);            // init DRAM refresh
   AWE32WD(PSST+30,0xFFFFFFE0);
   AWE32WD(CSL+30, 0x00FFFFE8);
   AWE32WD(PTRX+30,0);
   AWE32WD(CPF+30,0);
   AWE32WD(CCCA+30,0x00FFFFE3);

   AWE32WW(DCYSUSV+31,0x80);
   AWE32WD(PSST+31,0x00FFFFF0);
   AWE32WD(CSL+31, 0x00FFFFF8);
   AWE32WD(PTRX+31,0xFF);
   AWE32WD(CPF+31,0x8000);
   AWE32WD(CCCA+31,0x00FFFFF3);

   _disable();
   outpw(AWE32Base+0x802,0x3E);
   outpw(AWE32Base,0);
   while(k==0)
       k = inpw(AWE32Base+0x802) & 0x10;
   while(k==0x10)
       k = inpw(AWE32Base+0x802) & 0x10;
   outpw(AWE32Base+2,0x4828);
   outpw(AWE32Base+0x802,0x3C);
   outpw(AWE32Base+0x400,0);
   _enable();

   AWE32WD(VTFT+30,0xFFFFFFFF);
   AWE32WD(VTFT+31,0xFFFFFFFF);
}


/****************************/
/* Enable AWE32 DRAM access */
/****************************/
static void EnableDRAM()
{
    int k;

    for(k=0; k<30; k++)
    {   AWE32WW(DCYSUSV+k,0x80);
        AWE32WD(VTFT+k,0);
        AWE32WD(CVCF+k,0);
        AWE32WD(PTRX+k,0x40000000);
        AWE32WD(CPF+k,0x40000000);
        AWE32WD(CCCA+k,0x04000000|((k&1)<<25));
        AWE32WD(PSST+k,0);
        AWE32WD(CSL+k,0);
    }
    AWE32Wait(2);
}


/*****************************/
/* Disable AWE32 DRAM access */
/*****************************/
static void DisableDRAM()
{
    int k;
    for(k=0; k<30; k++)
    {   // disable DRAM access
        AWE32WD(CCCA+k,0);
        AWE32WW(DCYSUSV+k,0x807F);
    }
    AWE32Wait(2);
}


static void CheckDRAM()
{
    EnableDRAM();

    AWE32WD(SMALW,0x200000);    // Address for writing
    AWE32WW(SMLD,0xAAAA);
    AWE32WW(SMLD,0x5555);
    DRAMSize=0;
    while(DRAMSize < 28*1024)
    {   // 28 MB is max onboard memory
        AWE32Wait(2);
        AWE32WD(SMALR,0x200000);
        AWE32RW(SMLD);
        if(AWE32RW(SMLD) != 0xAAAA) break;
        if(AWE32RW(SMLD) != 0x5555) break;
        DRAMSize += 32;  // Size in KByte
        AWE32WD(SMALW,0x200000+DRAMSize*512);
        AWE32WW(SMLD,0xFFFF);
        AWE32WW(SMLD,0xFFFF);
    }
    DisableDRAM();
}


void aweSampUploadHandle(handle)
{
    SLONG loopstart = AWEsamp[handle].loopstart;
    SLONG loopend   = AWEsamp[handle].loopend;
    int   flags     = AWEsamp[handle].flags;
    SWORD *samp     = AWEsamp[handle].samp;
    int   i;

    AWE32WD(SMALW,AWEsamp[handle].base+0x200000);  // set awe write address

    for(i=0; i<loopend; i++) AWE32WW(SMLD, samp[i]);

    if(flags&SF_BIDI)
    {   for(i=0; i<(loopend-loopstart); i++)
        {   AWE32WW(SMLD,samp[loopend-i]);
        }
    }

    if(flags&SF_LOOP)
    {   // looping sample ?
        for(i=0; i<16; i++)
        {   AWE32WW(SMLD,samp[loopstart+i]);
        }
    } else
    {   for(i=0; i<16; i++)
        {   AWE32WW(SMLD,0);
        }
    }
}


static ULONG AWE_POOL;
static ULONG AWE_POOLMAX;


static void aweMemInit(void)
{
    CheckDRAM();
    AWE_POOL = 0;
    AWE_POOLMAX = DRAMSize<<9;
}


static SLONG aweMalloc(ULONG reqsize)
{
    int i;

    if((AWE_POOL+reqsize+16) >= AWE_POOLMAX) return 0;
    i = AWE_POOL;
    AWE_POOL += reqsize+16; // always 16 more samples

    return i;
}


static void aweFree(int handle)
{
    AWEsamp[handle].base = -1;
    while((AWEsampNo>0)&&(AWEsamp[AWEsampNo-1].base==-1)) AWEsampNo--;
    if(!AWEsampNo)
    {   AWE_POOL = 0;
    } else
    {   AWE_POOL = AWEsamp[AWEsampNo-1].base+AWEsamp[AWEsampNo-1].size;
    }
}


static int aweDetect()
{
    if(!SB_IsThere() || (AWE32Base==0xffff)) return 0;
    if(!AWE32Detect()) return 0;

    return 1;
}


static void aweSetPan(int GChan, int pan)
{
    pan = 255-pan;
    pan <<= 24L;
    AWE32WD(PSST+GChan, pan | (AWE32RD(PSST+GChan) & 0x00FFFFFF));
}


static void aweSetVolume(int GChan, int vol)
{
    AWE32WW(IFATN+GChan,0xFF00 | VolumeTable[vol]); // filter cutoff & volume
    AWE32WW(LFO1VAL+GChan,0x8000);                  // LFO1 delay
    AWE32WW(LFO2VAL+GChan,0x8000);                  // LFO2 delay
    AWE32WW(PEFE+GChan,0);                          // envelope 2 to pitch & filter
    AWE32WW(FMMOD+GChan,0);                         // LFO1 to pitch & filter
}


static void aweSetFreq(int GChan, int frq)
{
    int i;
    int f,fm;

    if(frq > 8192)
    {   // calculate using high-frequency table
        frq-=8191;
        f = frq&127; fm = 128-f;
        i = frq>>7;
        if(i>=FrqMax) i = FrqMax-1;
        i = ((int)HighFrqTable[i]*fm+(int)HighFrqTable[i+1]*f)>>7;  //-- linear interpolation
    } else
    {   f = frq&15; fm = 16-f;
        i = frq>>4;
        if(i<1) i = 1;
        i = ((int)LowFrqTable[i]*fm+(int)LowFrqTable[i+1]*f)>>4;  //-- linear interpolation
    }

    AWE32WW(IP+GChan,i);
}


/******************************************/
/* Play a note with settings from Chan[i] */
/******************************************/
static void aweStartVoice(int GChan, ULONG base, ULONG start,
                          ULONG loopstart, ULONG loopend, int flags)
{
    if (flags&SF_BIDI) loopend+=loopend-loopstart;
    if (!(flags&SF_LOOP)) loopend+=12;

    AWE32WW(ENVVOL+GChan,0x8000);
    AWE32WW(ENVVAL+GChan,0x8000);
    AWE32WW(DCYSUS+GChan,0x7F7F);
    AWE32WW(ATKHLDV+GChan,0x7F7F);
//   AWE32WW(LFO1VAL+GChan,0x8000);
    AWE32WW(ATKHLD+GChan,0x7F7F);
//   AWE32WW(LFO2VAL+GChan,0x8000);
//   AWE32WW(IP+GChan,0xE000);
//   AWE32WW(IFATN+GChan,0xFF00);
//   AWE32WW(PEFE+GChan,0);
//   AWE32WW(FMMOD+GChan,0);
    AWE32WW(TREMFRQ+GChan,0x10);
    AWE32WW(FM2FRQ2+GChan,0x10);

    base+=0x200000;
    AWE32WD(PSST+GChan,loopstart+base);
    AWE32WD(CSL+GChan,loopend+base);
    AWE32WD(CCCA+GChan,start+base);

    AWE32WD(VTFT+GChan,0x0000FFFF);
    AWE32WD(CVCF+GChan,0x0000FFFF);
    AWE32WW(DCYSUSV+GChan,0x7F7F);
    AWE32WD(PTRX+GChan,0x40000000);
    AWE32WD(CPF+GChan,0x40000000);
}


static void aweNoteOff(UBYTE Channel)
{
    AWE32WW(DCYSUSV+Channel,0x807F);
    AWE32WW(DCYSUS+Channel,0x807F);
}


static void aweNoteOffImm(int t)
{
    AWE32WW(DCYSUSV+t,0x80);
    AWE32WD(VTFT+t,0);
    AWE32WD(CVCF+t,0);
    AWE32WD(PTRX+t,0);
    AWE32WD(CPF+t,0);
}


static int awePosition(UBYTE channel)
{
    return (AWE32RD(CCCA+channel)&0xFFFFFF)-0x200000; // 24 bit adress space
}

/***************************************************************************
>>>>>>>>>>>>>>>>>>>>>>>>> The actual AWE driver <<<<<<<<<<<<<<<<<<<<<<<<<<<<
***************************************************************************/

void AWE_Update(void)
{
    UBYTE t;
    GHOLD *aud;
    ULONG base;

    if(AWE_BPM != md_bpm)
    {    SB_SetBPM(md_bpm);
         AWE_BPM = md_bpm;
    }

    for(t=0; t<awe_voices; t++)
    {   aud = &ghld[t];

        if(aud->pan != aud->panold)
        {                               // when panning changes => vol 0
            aweSetVolume(t,0);          // remove (some) clicks!
        }

        base = AWEsamp[aud->handle].base;

        if((!(aud->flags & SF_LOOP)) && aud->active)
            aud->active = awePosition(t)-base < aud->size;

        if(aud->kick)
        {   aud->kick = 0;

            aweNoteOffImm(t);

            if(aud->start != -1)
            {   if (aud->flags & SF_LOOP)
                {   aweStartVoice(t,base,aud->start,aud->reppos,aud->repend,aud->flags);
                } else
                {   aweStartVoice(t,base,aud->start,aud->size,aud->size,aud->flags);
                }

                aud->active = 1;
                aweSetFreq(t,aud->frq);
                aweSetPan(t,aud->pan);
                aweSetVolume(t,aud->vol);
            }
        } else
        {   aweSetFreq(t,aud->frq);
            aweSetPan(t,aud->pan);
            aweSetVolume(t,aud->vol);
        }
        aud->panold = aud->pan;
    }

    md_player();

    for(t=0; t<awe_voices; t++)
    {   aud = &ghld[t];
        if(aud->kick)
        {  aweNoteOff(t);
        }
    }
}


static PVI oldint;

static void interrupt newint(MIRQARGS)
{
    static int newTimer;
    int        oldTimer;
    long       diff;

    SB_StartSilence();
    inportb(DSP_DATA_AVAIL);

    if(sb_mode & DMODE_16BITS)
       inportb(sb_port + 0xf);
    else
       inportb(DSP_DATA_AVAIL);

    oldTimer = newTimer;
    newTimer = aweTimerGet();
    diff = aweTimerDiff(newTimer, oldTimer);
    if(diff > awe_timerReq) sb_silenceLenAdd--;
    if(diff < awe_timerReq) sb_silenceLenAdd++;

    MIrq_EOI(sb_irq);
    AWE_Update();
}



SWORD AWE_Load(SAMPLOAD *s, int type)
{
    int    handle;
    SWORD  *samp;
    int    l, adr;
    UWORD  flags;
    SLONG  loopend, loopstart;


    // Find empty slot to put sample address in
    
    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
       if(AWEsamp[handle].base==-1) break;

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        return -1;
    }

    SL_Sample8to16(s);  SL_SampleSigned(s);

    if((samp = malloc(s->sample->length * 2)) == NULL)
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        return -1;
    }

    loopstart = s->sample->loopstart;
    loopend   = s->sample->loopend;
    flags     = s->sample->flags;

    SL_Load(samp, s, s->sample->length);

    if(!(flags & SF_LOOP)) loopend = s->sample->length;
    l = loopend;
    if(flags & SF_BIDI) l += loopend - loopstart;
    adr = aweMalloc(l);

    AWEsamp[handle].base      = adr;
    AWEsamp[handle].size      = l;
    AWEsamp[handle].loopstart = loopstart;
    AWEsamp[handle].loopend   = loopend;
    AWEsamp[handle].flags     = flags;
    AWEsamp[handle].samp      = samp;

    EnableDRAM();
    aweSampUploadHandle(handle);
    DisableDRAM();

    free(samp);  samp = NULL;
    
    return handle;
}


void AWE_UnLoad(SWORD handle)
{
    aweFree(handle);
    if(AWEsamp[handle].samp!=NULL) free(AWEsamp[handle].samp);
    AWEsamp[handle].samp = NULL;
    AWEsamp[handle].base = -1;    
}


BOOL AWE_Init(void)
{
    int k;

    //md_mode |= DMODE_16BITS;          // awe can't do 8 bit mixing
    //md_mode &= ~(DMODE_SOFT_SNDFX | DMODE_SOFT_MUSIC | DMODE_SURROUND);

    if(sb_hidma == 0xff)
    {   _mm_errno = MMERR_INITIALIZING_DRIVER;
        return 1;
    }

    if((AWEsamp = malloc(MAXSAMPLEHANDLES * sizeof(TAWEsamp))) == NULL) return 1;
    for(k=0; k<MAXSAMPLEHANDLES; k++) AWEsamp[k].base = -1;

    SB_ResetDSP();

    AWE32Init(); aweMemInit();
    AWE_BPM = 125; SB_SetBPM(125);

    oldint  = MIrq_SetHandler(sb_irq, newint);

    return 0;
}


void AWE_Exit(void)
{
    MIrq_OnOff(sb_irq, 0);
    MIrq_SetHandler(sb_irq, oldint);
    AWE32Init();

    SB_SpeakerOff();
    SB_ResetDSP(); SB_ResetDSP();

    if(SB_DMAMEM != NULL) MDma_FreeMem(SB_DMAMEM);
    if(ghld!=NULL) free(ghld);
    if(AWEsamp!=NULL) free(AWEsamp);
    SB_DMAMEM = NULL;
    ghld      = NULL;
    AWEsamp   = NULL;
}


BOOL AWE_SetNumVoices(void)
{
    if(ghld!=NULL) free(ghld);
    if((awe_voices = md_hardchn) != 0)
    {   if((ghld = calloc(sizeof(GHOLD),awe_voices)) == NULL) return 1;
    } else
        ghld = NULL;

    // update the sb_mode variable to the current md_mode settings

    sb_mode &= ~(DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX);
    //sb_mode |= md_mode & (DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX);

    // Initialize the software mixer!

    return 0;
}


BOOL AWE_PlayStart(void)
{
    int t;

    for(t=0; t<awe_voices; t++)
    {   ghld[t].flags  = 0;
        ghld[t].handle = 0;
        ghld[t].kick   = 0;
        ghld[t].active = 0;
        ghld[t].frq    = 10000;
        ghld[t].vol    = 0;
        ghld[t].pan    = (t&1) ? 0 : 255;
    }

    SB_ResetDSP();
    MIrq_OnOff(sb_irq,1);
    SB_WriteDSP(0x40);
    SB_WriteDSP(sb_TC);
    SB_StartSilence();
    SB_SpeakerOn();

    return 0;
}


void AWE_PlayStop(void)
{
    int t;
    SB_SpeakerOff();
    MIrq_OnOff(sb_irq,0);
    for(t=0; t<30; t++) aweNoteOff(t);
}


BOOL AWE_IsThere(void)
{
    BOOL det = aweDetect();

    if(det)
    {   AWE32Init();
        CheckDRAM();
        return DRAMSize >= 512;
    }

    return 0;
}


BOOL AWE_Reset(void)
{
    return 0;
}


void AWE_VoiceSetVolume(UBYTE voice, UWORD vol)
{
    ghld[voice].vol = vol;
}


void AWE_VoiceSetFrequency(UBYTE voice,ULONG frq)
{
    ghld[voice].frq = frq;
}


void AWE_VoiceSetPanning(UBYTE voice, ULONG pan)
{
    if((pan == PAN_SURROUND) || !(md_mode & DMODE_STEREO)) pan = 128;
    ghld[voice].pan = pan;
}


void AWE_VoicePlay(UBYTE voice, SWORD handle, ULONG start, ULONG size, ULONG reppos, ULONG repend, UWORD flags)
{
    ghld[voice].flags  = flags;
    ghld[voice].handle = handle;
    ghld[voice].start  = start;
    ghld[voice].size   = size;
    ghld[voice].reppos = reppos;
    ghld[voice].repend = repend;
    ghld[voice].kick   = 1;
    ghld[voice].active = 1;
}


void AWE_VoiceStop(UBYTE voice)
{
    ghld[voice].start  = -1;
    ghld[voice].kick   = 1;
    ghld[voice].active = 0;
}


BOOL AWE_VoiceStopped(UBYTE voice)
{
    return !ghld[voice].active;
}


void AWE_VoiceReleaseSustain(UBYTE voice)
{

}


SLONG AWE_VoiceGetPosition(UBYTE voice)
{
    return awePosition(voice)-AWEsamp[ghld[voice].handle].base;
}


ULONG AWE_VoiceRealVolume(UBYTE voice)
{
    return 0;
}


void AWE_Dummy(void)
{
}


ULONG AWE_FreeMemory(int type)
{
    return ((AWE_POOLMAX - AWE_POOL)>>9)-2;
}


ULONG AWE_SampleLength(int type, SAMPLE *s)
{
    ULONG retval;

    retval = s->length * 2;
    if(s->flags & SF_BIDI) retval += (s->loopend-s->loopstart)*2;

    return retval + 64;
}


MDRIVER drv_awe =
{  "Sound Blaster AWE32",
   "Sound Blaster AWE32 Driver v1.2 by Amoon/vanic",
   30, 0,

   NULL,
   AWE_IsThere,
   AWE_Load,
   AWE_UnLoad,
   AWE_FreeMemory,
   AWE_SampleLength,
   AWE_Init,
   AWE_Exit,
   AWE_Reset,
   AWE_SetNumVoices,
   AWE_PlayStart,
   AWE_PlayStop,
   AWE_Dummy,
   AWE_VoiceSetVolume,
   AWE_VoiceSetFrequency,
   AWE_VoiceSetPanning,
   AWE_VoicePlay,
   AWE_VoiceStop,
   AWE_VoiceStopped,
   AWE_VoiceReleaseSustain,
   AWE_VoiceGetPosition,
   AWE_VoiceRealVolume
};


