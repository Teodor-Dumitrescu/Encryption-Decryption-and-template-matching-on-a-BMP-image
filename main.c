#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef struct
{
    double corelatie;
    unsigned int x,y,w,h;
    char cifra[20];
}detectie;

void grayscale_image(char* nume_fisier_sursa,char* nume_fisier_destinatie)
{
   FILE *fin, *fout;
   unsigned int dim_img, latime_img, inaltime_img;
   unsigned char pRGB[3], header[54], aux;

   //printf("nume_fisier_sursa = %s \n",nume_fisier_sursa);

   fin = fopen(nume_fisier_sursa, "rb");
   if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(unsigned int), 1, fin);
   //printf("Dimensiunea imaginii in octeti: %u\n", dim_img);

   fseek(fin, 18, SEEK_SET);
   fread(&latime_img, sizeof(unsigned int), 1, fin);
   fread(&inaltime_img, sizeof(unsigned int), 1, fin);
   //printf("Dimensiunea imaginii in pixeli (latime x inaltime): %u x %u\n",latime_img, inaltime_img);

   //copiaza octet cu octet imaginea initiala in cea noua
	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
		fflush(fout);
	}
	fclose(fin);

	//calculam padding-ul pentru o linie
	int padding;
    if(latime_img % 4 != 0)
        padding = 4 - (3 * latime_img) % 4;
    else
        padding = 0;

    //printf("padding = %d \n",padding);

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < inaltime_img; i++)
	{
		for(j = 0; j < latime_img; j++)
		{
			//citesc culorile pixelului
			fread(pRGB, 3, 1, fout);
			//fac conversia in pixel gri
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padding,SEEK_CUR);
	}
	fclose(fout);
}

void xorshift32(unsigned int **nr_aleatoare,int seed,int nr)
{
    *nr_aleatoare=(unsigned int*)malloc(sizeof(unsigned int)*nr);
    unsigned int r=seed,i;
    for(i=1;i<nr;i++)
    {
        r=r^r<<13;
        r=r^r>>17;
        r=r^r<<5;
        (*nr_aleatoare)[i]=r;
    }
}

void imagine_la_vector(char *s,unsigned char **imagine,unsigned int *W,unsigned int *H,unsigned int *padding,unsigned char **header)
{
    int i,j;
    FILE *f=fopen(s,"rb");
    if(f==NULL)
    {
        printf("Nu s-a reusit deschiderea fisierului %s din functia imagine_la_vector\n",s);
        return;
    }

    fseek(f,18,SEEK_SET);
    fread(W,sizeof(unsigned int),1,f);
    fread(H,sizeof(unsigned int),1,f);

    *padding=(4-(*W*3)%4)%4;

    *imagine=(unsigned char*)malloc(sizeof(unsigned char)*(*W)*(*H)*3);

    fseek(f,0,SEEK_SET);
    *header=(unsigned char*)malloc(54);
    fread(*header,1,54,f);

    fseek(f,54,SEEK_SET);
    for(i=(*H)-1;i>=0;i--)
    {
        for(j=0;j<*W;j++)
        {
            fread((*imagine+3*(i*(*W)+j)),3,1,f);
        }
        fseek(f,*padding,SEEK_CUR);
        //printf("%d\n",i);
    }
    fclose(f);
}

void vector_la_imagine(char *s,unsigned char *imagine,unsigned int W,unsigned int H,unsigned int padding,unsigned char *header)
{
    int i;
    unsigned char octet=0;
    FILE *g=fopen(s,"wb");
    fwrite(header,1,54,g);
    for(i=H-1;i>=0;i--)
    {
        fwrite(imagine+i*W*3,1,W*3,g);
        fwrite(&octet,1,padding,g);
    }
    fclose(g);
}
void chi_patrat(char *nume_imagine)
{
    unsigned char *imagine=NULL,*header=NULL;
    unsigned int W,H,padding;
    imagine_la_vector(nume_imagine,&imagine,&W,&H,&padding,&header);

    double chi_R=0,chi_G=0,chi_B=0,frecventa_estimata=W*H/256;
    int chi_r_aux,chi_g_aux,chi_b_aux;
    int i,j;
    for(i=0;i<=255;i++)
    {
        chi_b_aux=0;
        chi_g_aux=0;
        chi_r_aux=0;
        for(j=0;j<W*H;j++)
        {
            if(imagine[3*j]==i)
                chi_b_aux++;
            if(imagine[3*j+1]==i)
                chi_g_aux++;
            if(imagine[3*j+2]==i)
                chi_r_aux++;
        }
        chi_B=chi_B+((chi_b_aux-frecventa_estimata)*(chi_b_aux-frecventa_estimata))/frecventa_estimata;
        chi_G=chi_G+((chi_g_aux-frecventa_estimata)*(chi_g_aux-frecventa_estimata))/frecventa_estimata;
        chi_R=chi_R+((chi_r_aux-frecventa_estimata)*(chi_r_aux-frecventa_estimata))/frecventa_estimata;
    }
    free(imagine);
    printf("Chi squared test values for the image %s:\nR:%0.2lf \nG:%0.2lf \nB:%0.2lf\n",nume_imagine,chi_R,chi_G,chi_B);
}

void criptare(char *s,char *s1,char *text)
{
    unsigned char *imagine=NULL,*header=NULL;
    unsigned int W,H,padding;

    imagine_la_vector(s,&imagine,&W,&H,&padding,&header);
    //printf("W=%u  H=%u  padding=%u \n",W,H,padding);

    FILE *ftext=fopen(text,"rt");
    if(ftext==NULL)
    {
        printf("Couldn't open the secret key file\n");
        return ;
    }
    unsigned int R0,SV,*nr_aleatoare;
    fscanf(ftext,"%u",&R0);
    fscanf(ftext,"%u",&SV);     //am citit cheia secreta R0 si SV
    fclose(ftext);
    //printf("R0=%u  SV=%u\n",R0,SV);
    xorshift32(&nr_aleatoare,R0,2*W*H);   //am generat cele 2*W*H numere aleatoare

    unsigned int aux,k,r;
    unsigned int *permutare;
    permutare=(unsigned int*)malloc(sizeof(unsigned int)*W*H);

    for(k=0;k<W*H;k++)
        permutare[k]=k;

    for(k=W*H-1;k>=1;k--)
    {
        r=(nr_aleatoare[W*H-k])%(k+1); // %(k+1) deoarece vreau ca numerele aleatoare sa fie maxim k
        aux=permutare[k];
        permutare[k]=permutare[r];
        permutare[r]=aux;
    }

    unsigned char *imagine2=(unsigned char*)malloc(sizeof(char)*W*H*3); //imaginea cu pixelii permutati
    for(k=0;k<W*H;k++)
    {
        imagine2[3*permutare[k]]=imagine[3*k];
        imagine2[3*permutare[k]+1]=imagine[3*k+1];
        imagine2[3*permutare[k]+2]=imagine[3*k+2];
    }
    free(imagine);
    free(permutare);
    imagine=imagine2;

    unsigned char *imagine3=(unsigned char*)malloc(sizeof(char)*W*H*3);

    imagine3[0]=((SV<<24)>>24)^imagine[0]^(((nr_aleatoare[W*H])<<24)>>24);
    imagine3[1]=((SV<<16)>>24)^imagine[1]^(((nr_aleatoare[W*H])<<16)>>24);
    imagine3[2]=((SV<<8)>>24)^imagine[2]^(((nr_aleatoare[W*H])<<8)>>24);
    for(k=3;k<=3*(W*H-1);k=k+3)
    {
        imagine3[k]=imagine3[k-3]^imagine[k]^(((nr_aleatoare[W*H+k/3])<<24)>>24);
        imagine3[k+1]=imagine3[k+1-3]^imagine[k+1]^(((nr_aleatoare[W*H+k/3])<<16)>>24);
        imagine3[k+2]=imagine3[k+2-3]^imagine[k+2]^(((nr_aleatoare[W*H+k/3])<<8)>>24);
    }
    free(imagine);
    imagine=imagine3;
    vector_la_imagine(s1,imagine,W,H,padding,header);
    free(imagine);
    free(nr_aleatoare);
    free(header);
}

void decriptare(char *s1,char *s2,char *text)
{
    unsigned char *imagine=NULL,*header=NULL;
    unsigned int W,H,padding;

    imagine_la_vector(s1,&imagine,&W,&H,&padding,&header);

    FILE *ftext=fopen(text,"rt");
    if(ftext==NULL)
    {
        printf("Couldn't open the secret key file at decryption\n");
        return ;
    }

    unsigned int R0,SV,*nr_aleatoare;
    fscanf(ftext,"%u",&R0);
    fscanf(ftext,"%u",&SV);
    fclose(ftext);

    xorshift32(&nr_aleatoare,R0,2*W*H);   //am generat cele 2*W*H numere aleatoare

    unsigned int aux,k,r;
    unsigned int *permutare;
    permutare=(unsigned int*)malloc(sizeof(unsigned int)*W*H);

    for(k=0;k<W*H;k++)
        permutare[k]=k;

    for(k=W*H-1;k>=1;k--)
    {
        r=(nr_aleatoare[W*H-k])%(k+1);
        aux=permutare[k];
        permutare[k]=permutare[r];
        permutare[r]=aux;
    }
    unsigned int *permutare_inversa=(unsigned int*)malloc(sizeof(unsigned int)*W*H);
    for(k=0;k<W*H;k++)
    {
        permutare_inversa[permutare[k]]=k;
    }
    free(permutare);

    unsigned char *imagine3=(unsigned char*)malloc(sizeof(char)*W*H*3); //imaginea decriptata

    imagine3[0]=((SV<<24)>>24)^imagine[0]^(((nr_aleatoare[W*H])<<24)>>24);
    imagine3[1]=((SV<<16)>>24)^imagine[1]^(((nr_aleatoare[W*H])<<16)>>24);
    imagine3[2]=((SV<<8)>>24)^imagine[2]^(((nr_aleatoare[W*H])<<8)>>24);
    for(k=3;k<=3*(W*H-1);k=k+3)
    {
        imagine3[k]=imagine[k-3]^imagine[k]^(((nr_aleatoare[W*H+k/3])<<24)>>24);
        imagine3[k+1]=imagine[k+1-3]^imagine[k+1]^(((nr_aleatoare[W*H+k/3])<<16)>>24);
        imagine3[k+2]=imagine[k+2-3]^imagine[k+2]^(((nr_aleatoare[W*H+k/3])<<8)>>24);
    }
    free(imagine);
    imagine=imagine3;

    unsigned char *imagine2=(unsigned char*)malloc(sizeof(char)*W*H*3);  //imaginea decriptata+permutata(prin permutarea inversa)
    for(k=0;k<W*H;k++)
    {
        imagine2[3*permutare_inversa[k]]=imagine[3*k];
        imagine2[3*permutare_inversa[k]+1]=imagine[3*k+1];
        imagine2[3*permutare_inversa[k]+2]=imagine[3*k+2];
    }
    free(imagine);
    imagine=imagine2;
    vector_la_imagine(s2,imagine,W,H,padding,header);
    free(imagine);
    free(permutare_inversa);
    free(nr_aleatoare);
    free(header);
}

void template_matching(char *nume_imagine,char *nume_sablon,double prag,detectie **vector_detectii,int *nr_detectii)
{
    unsigned char *imagine=NULL,*header_img=NULL,*sablon=NULL,*header_sab=NULL;
    unsigned int W_img,H_img,padding_img;
    unsigned int W_sab,H_sab,padding_sab;

    char *nume_imagine_grey="test_grayscale.bmp";
    char *nume_sablon_grey="sablon_grey.bmp";

    grayscale_image(nume_imagine,nume_imagine_grey);
    grayscale_image(nume_sablon,nume_sablon_grey);

    imagine_la_vector(nume_imagine_grey,&imagine,&W_img,&H_img,&padding_img,&header_img);
    imagine_la_vector(nume_sablon_grey,&sablon,&W_sab,&H_sab,&padding_sab,&header_sab);

    *vector_detectii=(detectie*)malloc(sizeof(detectie)*H_img*W_img);

    *nr_detectii=0;
    int i,j,k,l;
    unsigned int n=W_sab*H_sab;
    double corelatie=0,medie_sablon=0,deviatie_sablon=0;
    double medie_fereastra,deviatie_fereastra;

    for(i=0;i<H_sab;i++)
        for(j=0;j<3*W_sab;j=j+3)
            medie_sablon=medie_sablon+sablon[3*i*W_sab+j];
    medie_sablon=medie_sablon/n; //S barat

    for(i=0;i<H_sab;i++)
    {
        for(j=0;j<3*W_sab;j=j+3)
        {
            deviatie_sablon=deviatie_sablon+(sablon[3*i*W_sab+j]-medie_sablon)*(sablon[3*i*W_sab+j]-medie_sablon);
        }
    }
    deviatie_sablon=deviatie_sablon/(n-1);
    deviatie_sablon=sqrt(deviatie_sablon);  //sigma S

    for(i=0;i<=H_img-H_sab;i++)
    {
        for(j=0;j<=3*(W_img-W_sab);j=j+3)
        {
            corelatie=0;
            medie_fereastra=0;
            deviatie_fereastra=0;
            for(k=i;k<=i+H_sab-1;k++)
            {
                for(l=j;l<=j+(W_sab-1)*3;l=l+3)
                {
                    medie_fereastra=medie_fereastra+imagine[3*k*W_img+l];
                }
            }
            medie_fereastra=medie_fereastra/n;   //f barat

            for(k=i;k<=i+H_sab-1;k++)
            {
                for(l=j;l<=j+(W_sab-1)*3;l=l+3)
                {
                    deviatie_fereastra=deviatie_fereastra+(imagine[3*k*W_img+l]-medie_fereastra)*(imagine[3*k*W_img+l]-medie_fereastra);
                }
            }
            deviatie_fereastra=deviatie_fereastra/n;
            deviatie_fereastra=sqrt(deviatie_fereastra);  //sigma f

            for(k=i;k<=i+H_sab-1;k++)
            {
                for(l=j;l<=j+(W_sab-1)*3;l=l+3)
                {
                    corelatie=corelatie+((imagine[3*k*W_img+l]-medie_fereastra)*(sablon[3*(k-i)*W_sab+(l-j)]-medie_sablon))/(deviatie_fereastra*deviatie_sablon);
                }
            }
            corelatie=corelatie/n;  //corelatia pentru fereastra (i,j)
            if(corelatie>=prag)
            {
                (*vector_detectii)[*nr_detectii].x=i;
                (*vector_detectii)[*nr_detectii].y=j;
                (*vector_detectii)[*nr_detectii].corelatie=corelatie;
                (*vector_detectii)[*nr_detectii].h=H_sab;
                (*vector_detectii)[*nr_detectii].w=W_sab;
                strcpy((*vector_detectii)[*nr_detectii].cifra,nume_sablon);
                (*nr_detectii)++;
            }
        }
    }
    realloc(*vector_detectii,sizeof(detectie)*(*nr_detectii));
}

void deseneaza_contur(char *nume_imagine,detectie fereastra,unsigned char culoare_B,unsigned char culoare_G,unsigned char culoare_R)
{
    unsigned char *imagine=NULL,*header_img=NULL,*sablon=NULL,*header_sab=NULL;
    unsigned int W_img,H_img,padding_img;

    imagine_la_vector(nume_imagine,&imagine,&W_img,&H_img,&padding_img,&header_img);

    int i,j;
    for(j=fereastra.y;j<=fereastra.y+3*(fereastra.w-1);j=j+3)
    {
        imagine[fereastra.x*W_img*3+j]=culoare_B;
        imagine[fereastra.x*W_img*3+j+1]=culoare_G;
        imagine[fereastra.x*W_img*3+j+2]=culoare_R;
        imagine[(fereastra.x+fereastra.h-1)*W_img*3+j]=culoare_B;
        imagine[(fereastra.x+fereastra.h-1)*W_img*3+j+1]=culoare_G;
        imagine[(fereastra.x+fereastra.h-1)*W_img*3+j+2]=culoare_R;
    }
    for(i=fereastra.x;i<=fereastra.x+fereastra.h-1;i++)
    {
        imagine[i*W_img*3+fereastra.y]=culoare_B;
        imagine[i*W_img*3+fereastra.y+1]=culoare_G;
        imagine[i*W_img*3+fereastra.y+2]=culoare_R;
        imagine[i*W_img*3+fereastra.y+3*(fereastra.w-1)]=culoare_B;
        imagine[i*W_img*3+fereastra.y+3*(fereastra.w-1)+1]=culoare_G;
        imagine[i*W_img*3+fereastra.y+3*(fereastra.w-1)+2]=culoare_R;
    }
    vector_la_imagine(nume_imagine,imagine,W_img,H_img,padding_img,header_img);
    free(imagine);
}

int cmp(const void *a,const void *b)
{
    if((*(detectie*)a).corelatie>=(*(detectie*)b).corelatie)
        return -1;
    return 1;
}

void sortare_detectii(detectie **vector_detectii,int nr_elem)
{
    qsort(*vector_detectii,nr_elem,sizeof(detectie),cmp);
}

int verificare_suprapunere(detectie f1,detectie f2)
{
    double suprapunere,arie1,arie2,arie_intersectie=0;
    int i,j;
    arie1=f1.h*f1.w;
    arie2=f2.h*f2.w;
    for(i=f1.x;i<=f1.x+f1.h-1;i++)
    {
        for(j=f1.y;j<=f1.y+3*(f1.w-1);j=j+3)
        {
            if(i>=f2.x && i<=f2.x+f2.h-1)
                if(j>=f2.y && j<=f2.y+3*(f2.w-1))
                    arie_intersectie+=1;
        }
    }
    suprapunere=arie_intersectie/(arie1+arie2-arie_intersectie);
    if(suprapunere>0.2)
        return 1;
    return 0;
}

void eliminare_nonmaxime(detectie **toate_detectiile,int *nr_detectii_total)
{
    int i,j;
    int aux_nr_detectii_total=*nr_detectii_total;
    for(i=0;i<*nr_detectii_total;i++)
    {
        if((*toate_detectiile)[i].corelatie!=-1)
        {
            for(j=i+1;j<*nr_detectii_total;j++)
            {
                if((*toate_detectiile)[j].corelatie!=-1 && verificare_suprapunere((*toate_detectiile)[i],(*toate_detectiile)[j])==1)
                {
                    (*toate_detectiile)[j].corelatie=-1;
                    aux_nr_detectii_total--;
                }
            }
        }
    }
    sortare_detectii(toate_detectiile,*nr_detectii_total);
    realloc(*toate_detectiile,sizeof(detectie)*aux_nr_detectii_total);
    *nr_detectii_total=aux_nr_detectii_total;
}

int dimensiune_imagine(char *imagine)
{
    unsigned int latime,inaltime;
    FILE *f=fopen(imagine,"rb");
    fseek(f,18,SEEK_SET);
    fread(&latime,sizeof(unsigned int),1,f);
    fread(&inaltime,sizeof(unsigned int),1,f);
    fclose(f);
    return latime*inaltime;
}
int main()
{
    int i,j;

    printf("<<<<ENCRYPTION>>>>\n");

    char nume_imagine_initiala[30];
    char nume_imagine_criptata[30];
    char nume_imagine_decriptata[30];
    char nume_fisier_cheie_secreta[30];

    printf("Enter the name of the image you want to encrypt: ");
    scanf("%s",nume_imagine_initiala);

    printf("Enter the new name for the encrypted image: ");
    scanf("%s",nume_imagine_criptata);

    printf("Enter the name of the secret key file: ");
    scanf("%s",nume_fisier_cheie_secreta);

    criptare(nume_imagine_initiala,nume_imagine_criptata,nume_fisier_cheie_secreta);

    printf("<<<<DECRYPTION>>>>\n");

    printf("Enter the name of the image that you want to decrypt: ");
    scanf("%s",nume_imagine_criptata);

    printf("Enter the new name for the decrypted image: ");
    scanf("%s",nume_imagine_decriptata);

    printf("Enter the name of the secret key file: ");
    scanf("%s",nume_fisier_cheie_secreta);

    decriptare(nume_imagine_criptata,nume_imagine_decriptata,nume_fisier_cheie_secreta);

    chi_patrat(nume_imagine_initiala);

    chi_patrat(nume_imagine_criptata);

    printf("<<<<TEMPLATE MATCHING>>>>\n");

    char nume_imagine[30];

    printf("Enter the name of the image on which the template matching operation will be run: ");
    scanf("%s",nume_imagine);

    int nr_sabloane;
    printf("Enter the number of templates: ");
    scanf("%d",&nr_sabloane);

    char **sabloane=(char**)malloc(sizeof(char*)*nr_sabloane);
    for(i=0;i<nr_sabloane;i++)
    {
        sabloane[i]=(char*)malloc(sizeof(char)*30);  //numele sabloanelor au pana in 30 caractere
    }
    for(i=0;i<nr_sabloane;i++)
    {
        printf("Enter the name of the template %d: ",i);
        scanf("%s",sabloane[i]);
    }

    double prag=0.50;
    int nr_detectii=0,nr_detectii_total=0; //primul pentru fiecare sablon, pe rand, al doilea pentru toate la un loc
    unsigned char B,G,R;
    detectie *vector_detectii,*toate_detectiile; //primul contine detectiile pentru un sablon, la un moment dat, al doilea pentru toate sabloanele

    unsigned int dimensiune; //numarul de pixeli ai imaginii
    dimensiune=dimensiune_imagine(nume_imagine);

    template_matching(nume_imagine,sabloane[0],prag,&vector_detectii,&nr_detectii);

    toate_detectiile=(detectie*)malloc(sizeof(detectie)*dimensiune*nr_sabloane); //numarul mai mult decat maxim de detectii pentru toate 10 sabloanele
    for(i=0;i<nr_detectii;i++)
    {
        toate_detectiile[nr_detectii_total+i].corelatie=vector_detectii[i].corelatie;
        toate_detectiile[nr_detectii_total+i].x=vector_detectii[i].x;
        toate_detectiile[nr_detectii_total+i].y=vector_detectii[i].y;
        toate_detectiile[nr_detectii_total+i].h=vector_detectii[i].h;
        toate_detectiile[nr_detectii_total+i].w=vector_detectii[i].w;
        strcpy(toate_detectiile[nr_detectii_total+i].cifra,vector_detectii[i].cifra);
    }
    nr_detectii_total=nr_detectii_total+nr_detectii;
    //initial facusem cu realloc dupa template_matching cu fiecare sablon(de aceea am separat
    //pentru sablonul 0 si pentru 1-9 intr-un for), dar facea urat programul asa ca am renuntat
    //si am facut un malloc mare si un realloc la final (am lasat asa sablonul 0, separat de celelalte)
    for(i=1;i<nr_sabloane;i++)
    {
        template_matching(nume_imagine,sabloane[i],prag,&vector_detectii,&nr_detectii);
        for(j=0;j<nr_detectii;j++)
        {
            toate_detectiile[nr_detectii_total+j].corelatie=vector_detectii[j].corelatie;
            toate_detectiile[nr_detectii_total+j].x=vector_detectii[j].x;
            toate_detectiile[nr_detectii_total+j].y=vector_detectii[j].y;
            toate_detectiile[nr_detectii_total+j].h=vector_detectii[j].h;
            toate_detectiile[nr_detectii_total+j].w=vector_detectii[j].w;
            strcpy(toate_detectiile[nr_detectii_total+j].cifra,vector_detectii[j].cifra);
        }
        nr_detectii_total=nr_detectii_total+nr_detectii;
    }
    realloc(toate_detectiile,sizeof(detectie)*nr_detectii_total);

    sortare_detectii(&toate_detectiile,nr_detectii_total);

    eliminare_nonmaxime(&toate_detectiile,&nr_detectii_total);

    for(i=0;i<nr_detectii_total;i++)
    {
        if(strcmp(toate_detectiile[i].cifra,sabloane[0])==0)
        {
            R=255;
            G=0;
            B=0;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[1])==0)
        {
            R=255;
            G=255;
            B=0;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[2])==0)
        {
            R=0;
            G=255;
            B=0;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[3])==0)
        {
            R=0;
            G=255;
            B=255;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[4])==0)
        {
            R=255;
            G=0;
            B=255;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[5])==0)
        {
            R=0;
            G=0;
            B=255;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[6])==0)
        {
            R=192;
            G=192;
            B=192;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[7])==0)
        {
            R=255;
            G=140;
            B=0;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[8])==0)
        {
            R=128;
            B=0;
            G=128;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
        if(strcmp(toate_detectiile[i].cifra,sabloane[9])==0)
        {
            R=128;
            G=0;
            B=0;
            deseneaza_contur(nume_imagine,toate_detectiile[i],B,G,R);
        }
    }
    free(vector_detectii);
    free(toate_detectiile);
    for(i=0;i<nr_sabloane;i++)
        free(sabloane[i]);
    free(sabloane);
    return 0;
}
