/*******************************************************************************
* Rapide programme pour transformer un bmp 16 couleurs en data                 *
********************************************************************************
* Author  : OlivierP                                                           *
* Date    : march & july 2012                                                  *
* License : GNU GPLv3 (http://www.gnu.org/copyleft/gpl.html)                   *
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
	Intensity       Red     Green   Blue    Colour  RGB        EGA no
	0       0       0       0       Black                   #000000    0
	0       0       0       1       Blue                    #0000aa    1
	0       0       1       0       Green                   #00aa00    2
	0       0       1       1       Cyan                    #00aaaa    3
	0       1       0       0       Red                     #aa0000    4
	0       1       0       1       Magenta                 #aa00aa    5
	0       1       1       0       Brown                   #aa5500    20
	0       1       1       1       White                   #aaaaaa    7
	1       0       0       0       Grey                    #555555    56
	1       0       0       1       Light Blue              #5555ff    57
	1       0       1       0       Light Green             #55ff55    58
	1       0       1       1       Light Cyan              #55ffff    59
	1       1       0       0       Light Red               #ff5555    60
	1       1       0       1       Light Magenta           #ff55ff    61
	1       1       1       0       Yellow                  #ffff55    62
	1       1       1       1       Intense White           #ffffff    63
*/
/* unsigned char PALET_CGA[] = {63, 62, 60, 58, 61, 60, 6, 56, 59, 5, 1, 4, 0, 57, 0, 1, 1}; */
unsigned char CGAI[] = {1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0};
unsigned char CGAR[] = {1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0};
unsigned char CGAG[] = {1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned char CGAB[] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1};

int main(int argc, char **argv)
{
	if (argc < 3)
		printf("usage : %s fichier.bmp nom\n", argv[0]);
	else
	{
		FILE *f = fopen(argv[1], "rb");
		if (f != NULL)
		{
			char w, h;
			int pitch, pos, i;

			printf("\nunsigned char %s\[\] = \{\n\t ", argv[2]);

			fseek(f, 18, SEEK_SET);
			w = fgetc(f);

			fseek(f, 22, SEEK_SET);
			h = fgetc(f);

			pitch = (w%8) ? ((w>>3)+1)<<3 : w;

			fseek(f, 0, SEEK_END);
			pos = ftell(f);

			for (i=h; i>0; i--)
			{
				unsigned char bp0=0, bp1=0, n=0;
				int j, reste;

				if (i!=h) printf(",");
				pos -= pitch/2;
				fseek(f, pos, SEEK_SET);
				for (j=0; j<w/2; j++)
				{
					unsigned char c = fgetc(f);

					unsigned char b0 = (c & 0xf0) >> 4;
					unsigned char b1 = (c & 0x0f);

					unsigned char p0bB = CGAB[b0];
					unsigned char p0bI = CGAI[b0];
					unsigned char p0bR = CGAR[b0];
					unsigned char p0bG = CGAG[b0];
					unsigned char p1bB = CGAB[b1];
					unsigned char p1bI = CGAI[b1];
					unsigned char p1bR = CGAR[b1];
					unsigned char p1bG = CGAG[b1];

					switch (n)
					{
					case 0:
						bp0 |= (p0bB << 7);
						bp0 |= (p0bI << 6);
						bp0 |= (p1bB << 5);
						bp0 |= (p1bI << 4);
						bp1 |= (p0bR << 7);
						bp1 |= (p0bG << 6);
						bp1 |= (p1bR << 5);
						bp1 |= (p1bG << 4);
						n = 4;
					case 1:
					case 2:
					case 3:
						break;
					case 4:
						bp0 |= (p0bB << 3);
						bp0 |= (p0bI << 2);
						bp0 |= (p1bB << 1);
						bp0 |= (p1bI << 0);
						bp1 |= (p0bR << 3);
						bp1 |= (p0bG << 2);
						bp1 |= (p1bR << 1);
						bp1 |= (p1bG << 0);
						n = 8;
					case 5:
					case 6:
					case 7:
						break;
					}
					if (n==8)
					{
						printf("0x%02x,", (int)bp1);
						printf("0x%02x,", (int)bp0);
						bp0 = bp1 = n = 0;
					}
				}
				printf(" ENDL,");
				reste = (i==1) ? 0 : 80 - w/4;
				printf("0x%02x\n\t", reste);
			}
			printf("\};\n");
			fclose(f);
		}
	else printf("impossible d'ouvrir %s\n", argv[1]);
	}
	return 0;
}
