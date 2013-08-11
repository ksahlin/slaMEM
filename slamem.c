#include <stdio.h>
#include <stdlib.h>
#include "tools.h"
#include "sequence.h"
#include "bwtindex.h"
#include "lcparray.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

//#define BENCHMARK 1
#if defined(unix) && defined(BENCHMARK)
#include "unistd.h"
#include "string.h"
#endif

#define PAUSE_AT_EXIT 1

void GetMEMs(int numRefs, int numSeqs, int minMemSize, int bothStrands, char *outFilename){
	FILE *memsOutputFile;
	int i, j, r, s, textsize, depth, memSize, numMems;
	long long int sumMemsSize, totalNumMems, totalAvgMemsSize;
	unsigned int topPtr, bottomPtr, prevTopPtr, prevBottomPtr, savedTopPtr, savedBottomPtr, n;
	char c, *text;
	unsigned char *lcpArray;
	int progressCounter, progressStep;
	#ifdef DEBUGMEMS
	char *reftext;
	int refsize, refpos;
	#endif
	#if defined(unix) && defined(BENCHMARK)
	char command[32];
	int commretval;
	#endif
	/*
	for(i=0;i<numSequences;i++){
		LoadSequenceChars(allSequences[i]);
		text=(allSequences[i]->chars);
		textsize=(allSequences[i]->size);
		lcpArray=NULL;
		FMI_NewBuildIndex(text,textsize,&lcpArray,1);
		j=BuildSampledLCPArray(text,textsize,lcpArray,1);
		FreeSampledSuffixArray();
		FMI_FreeIndex();
		if(lcpArray!=NULL) free(lcpArray);
		FreeSequenceChars(allSequences[0]);
		break;
	}
	DeleteAllSequences();
	getchar();
	exit(0);
	*/
	printf("> Using options: minimum MEM length = %d\n",minMemSize);
	memsOutputFile=fopen(outFilename,"w");
	if(memsOutputFile==NULL){
		printf("\n> ERROR: Cannot create output file <%s>\n",outFilename);
		exit(-1);
	}
	for(r=0;r<numRefs;r++){ // process all references
		printf("> Processing reference sequence \"%s\" ...\n",(allSequences[r]->name));
		fflush(stdout);
		LoadSequenceChars(allSequences[r]);
		text=(allSequences[r]->chars);
		textsize=(allSequences[r]->size);
		lcpArray=NULL;
		FMI_NewBuildIndex(text,textsize,&lcpArray,1);
		i=BuildSampledLCPArray(text,textsize,lcpArray,1);
		if(lcpArray!=NULL) free(lcpArray);
		#ifndef DEBUGMEMS
		FreeSequenceChars(allSequences[r]);
		#else
		reftext=(allSequences[r]->chars);
		refsize=(allSequences[r]->size);
		#endif
		#if defined(unix) && defined(BENCHMARK)
		if(numRefs==1){
			sprintf(command,"memusgpid %d &",(int)getpid());
			commretval=system(command);
		}
		#endif
		printf("> Matching query sequences against index ...\n");
		fflush(stdout);
		totalNumMems=0;
		totalAvgMemsSize=0;
		for(i=numRefs;i<numSeqs;i++){ // process all queries
			LoadSequenceChars(allSequences[i]);
			text=(allSequences[i]->chars);
			textsize=(allSequences[i]->size);
			progressStep=(textsize/10);
			for(s=0;s<=bothStrands;s++){ // process one or both strands
				if(s==0){ // forward strand
					printf(":: \"%s\" ",(allSequences[i]->name));
					fprintf(memsOutputFile,">%s\n",(allSequences[i]->name));
				} else { // reverse strand
					ReverseComplementSequence(text,textsize); // convert to reverse strand
					printf(":: \"%s Reverse\" ",(allSequences[i]->name));
					fprintf(memsOutputFile,">%s Reverse\n",(allSequences[i]->name));
				}
				fflush(stdout);
				progressCounter=0;
				numMems=0;
				sumMemsSize=0;
				depth=0;
				topPtr=0;
				bottomPtr=FMI_GetBWTSize();
				prevTopPtr=topPtr;
				prevBottomPtr=bottomPtr;
				for(j=(textsize-1);j>=0;j--){
					if(progressCounter==progressStep){ // print progress dots
						putchar('.');
						fflush(stdout);
						progressCounter=0;
					} else progressCounter++;
					while( (n=FMI_FollowLetter(text[j],&topPtr,&bottomPtr))==0 ){ // when no match exits, follow prefix links to broaden the interval
						topPtr = prevTopPtr; // restore pointer values, because they got lost when no hits exist
						bottomPtr = prevBottomPtr;
						depth = GetEnclosingLCPInterval(&topPtr,&bottomPtr); // get enclosing interval and corresponding destination depth
						if( depth == -1 ) break; // can happen for example when current seq contains 'N's but the indexed reference does not
						prevTopPtr = topPtr; // save pointer values in case the match fails again
						prevBottomPtr = bottomPtr;
					}
					depth++;
					if( depth >= minMemSize ){
						savedTopPtr = topPtr; // save the original interval to restore after finished processing MEMs
						savedBottomPtr = bottomPtr;
						prevTopPtr = (bottomPtr+1); // to process the first interval entirely
						prevBottomPtr = bottomPtr;
						memSize = depth;
						if( j != 0 ) c = text[j-1]; // next char to be processed (to the left)
						else c = '\0';
						while( memSize >= minMemSize ){ // process all parent intervals down to this size limit
							for( n = topPtr ; n != prevTopPtr ; n++ ){ // from topPtr down to prevTopPtr
								if( FMI_GetCharAtBWTPos(n) != c ){
									#ifndef DEBUGMEMS
									fprintf(memsOutputFile,"%d\t%d\t%d\n",(int)(FMI_PositionInText(n)+1),(j+1),memSize);
									#else
									refpos = (int)FMI_PositionInText(n);
									fprintf(memsOutputFile,"%d\t%d\t%d",(refpos+1),(j+1),memSize);
									fputc('\t',memsOutputFile);
									fputc((refpos==0)?('$'):(reftext[refpos-1]+32),memsOutputFile);
									fprintf(memsOutputFile,"%.*s...%.*s",4,(char *)(reftext+refpos),4,(char *)(reftext+refpos+memSize-4));
									fputc(((refpos+memSize)==refsize)?('$'):(reftext[refpos+memSize]+32),memsOutputFile);
									fputc('\t',memsOutputFile);
									fputc((j==0)?('$'):(text[j-1]+32),memsOutputFile);
									fprintf(memsOutputFile,"%.*s...%.*s",4,(char *)(text+j),4,(char *)(text+j+memSize-4));
									fputc(((j+memSize)==textsize)?('$'):(text[j+memSize]+32),memsOutputFile);
									fputc('\n',memsOutputFile);
									#endif
									numMems++;
									sumMemsSize += memSize;
								}
							}
							for( n = bottomPtr ; n != prevBottomPtr ; n-- ){ // from bottomPtr up to prevBottomPtr
								if( FMI_GetCharAtBWTPos(n) != c ){
									#ifndef DEBUGMEMS
									fprintf(memsOutputFile,"%d\t%d\t%d\n",(int)(FMI_PositionInText(n)+1),(j+1),memSize);
									#else
									refpos = (int)FMI_PositionInText(n);
									fprintf(memsOutputFile,"%d\t%d\t%d",(refpos+1),(j+1),memSize);
									fputc('\t',memsOutputFile);
									fputc((refpos==0)?('$'):(reftext[refpos-1]+32),memsOutputFile);
									fprintf(memsOutputFile,"%.*s...%.*s",4,(char *)(reftext+refpos),4,(char *)(reftext+refpos+memSize-4));
									fputc(((refpos+memSize)==refsize)?('$'):(reftext[refpos+memSize]+32),memsOutputFile);
									fputc('\t',memsOutputFile);
									fputc((j==0)?('$'):(text[j-1]+32),memsOutputFile);
									fprintf(memsOutputFile,"%.*s...%.*s",4,(char *)(text+j),4,(char *)(text+j+memSize-4));
									fputc(((j+memSize)==textsize)?('$'):(text[j+memSize]+32),memsOutputFile);
									fputc('\n',memsOutputFile);
									#endif
									numMems++;
									sumMemsSize += memSize;
								}
							}
							prevTopPtr = topPtr;
							prevBottomPtr = bottomPtr;
							memSize = GetEnclosingLCPInterval(&topPtr,&bottomPtr); // get parent interval and its depth
						}
						topPtr = savedTopPtr;
						bottomPtr = savedBottomPtr;
					}
					prevTopPtr=topPtr; // save pointer values in case there's no match on the next char, and they loose their values
					prevBottomPtr=bottomPtr;
				} // end of loop for all chars of seq
				memSize=(int)(numMems==0)?(0):(sumMemsSize/(long long)numMems);
				totalNumMems+=numMems;
				totalAvgMemsSize+=memSize;
				printf(" (%d MEMs ; avg size = %d bp)\n",numMems,memSize);
				fflush(stdout);
			} // end of loop for both strands
			FreeSequenceChars(allSequences[i]);
		} // end of loop for all queries
		FMI_FreeIndex();
		FreeSampledSuffixArray();
		if((numSeqs-numRefs)!=1){ // if more than one query, print average stats for all queries
			printf(":: Average %d MEMs found per sequence (avg size = %d bp)\n",(int)(totalNumMems/(numSeqs-numRefs)),(int)(totalAvgMemsSize/(numSeqs-numRefs)));
		}
		fflush(stdout);
	} // end of loop for all references
	printf("> Saving MEMs to <%s> ... ",outFilename);
	fclose(memsOutputFile);
	printf("OK\n");
	fflush(stdout);
}

typedef struct {
	int refPos;
	int queryPos;
	int size;
} MEMInfo;

int MEMInfoSortFunction(const void *a, const void *b){
	return ( (((MEMInfo *)a)->refPos) - (((MEMInfo *)b)->refPos) );
}

void SortMEMsFile(char *memsFilename){
	FILE *memsFile, *sortedMemsFile;
	char c, *sortedMemsFilename, seqname[256];
	int numMems, maxNumMems, numSeqs, refpos, querypos, memsize;
	MEMInfo *memsArray;
	printf("> Sorting MEMs from <%s> ...\n",memsFilename);
	fflush(stdout);
	if((memsFile=fopen(memsFilename,"r"))==NULL){
		printf("\n> ERROR: Cannot read file\n");
		exit(-1);
	}
	sortedMemsFilename=AppendToBasename(memsFilename,"-sorted.txt");
	if((sortedMemsFile=fopen(sortedMemsFilename,"w"))==NULL){
		printf("\n> ERROR: Cannot write file\n");
		exit(-1);
	}
	seqname[0]='\0';
	refpos=-1;
	querypos=-1;
	memsize=-1;
	numSeqs=0;
	numMems=0;
	maxNumMems=0;
	memsArray=NULL;
	while(1){
		c=fgetc(memsFile);
		if(c=='>' || c==EOF){
			if(numSeqs!=0){
				printf("(%d MEMs)\n",numMems);
				fflush(stdout);
				qsort(memsArray,numMems,sizeof(MEMInfo),MEMInfoSortFunction);
				fprintf(sortedMemsFile,">%s\n",seqname);
				while(numMems!=0){
					numMems--;
					refpos=memsArray[numMems].refPos;
					querypos=memsArray[numMems].queryPos;
					memsize=memsArray[numMems].size;
					fprintf(sortedMemsFile,"%d\t%d\t%d\n",refpos,querypos,memsize);
				}
			}
			if(c==EOF) break;
			numMems=0;
			fscanf(memsFile," %255[^\n]\n",seqname);
			printf(":: '%s' ... ",seqname);
			fflush(stdout);
			numSeqs++;
			continue;
		} else ungetc(c,memsFile);
		if((fscanf(memsFile," %d %d %d ",&refpos,&querypos,&memsize))!=3){
			printf("\n> ERROR: Invalid format\n");
			getchar();
			exit(-1);
		}
		if(numMems==maxNumMems){
			maxNumMems+=1024;
			memsArray=(MEMInfo *)realloc(memsArray,(maxNumMems*sizeof(MEMInfo)));
		}
		memsArray[numMems].refPos=refpos;
		memsArray[numMems].queryPos=querypos;
		memsArray[numMems].size=memsize;
		numMems++;
	}
	printf("> Saving sorted MEMs to <%s> ...\n",sortedMemsFilename);
	fflush(stdout);
	fclose(memsFile);
	fclose(sortedMemsFile);
	free(sortedMemsFilename);
	free(memsArray);
	printf("> Done!\n");
	getchar();
	exit(0);
}

// TODO: create ref alignment plot from mems file with both strands
// TODO: output Multi-MEMS
int main(int argc, char *argv[]){
	int i, n, mode, numFiles, numSeqsInFirstFile, outFileArgNum, argBothStrands, argNoNs, argMinMemSize;
	char *outFilename;
	printf("[ slaMEM v0.5 ]\n");
	if(argc<3){
		printf("\nUsage:\n");
		printf("\t%s <options> <reference_file> <query_files>",argv[0]);
		printf("\nOptions:\n");
		printf("\t-m\tfind MEMs (default)\n");
		printf("\t-l\tminimum match length (default=50)\n");
		printf("\t-o\toutput file name (default=\"*-mems.txt\")\n");
		printf("\t-b\tprocess both strands\n");
		printf("\t-n\tdiscard N's\n");
		printf("\nExtra:\n");
		//printf("\t-s\tsort MEMs file\n");
		printf("\t-v\tgenerate MEMs map image\n");
		printf("\n");
		return (-1);
	}
	if( ParseArgument(argc,argv,"S",0) ){ // Sort MEMs
		if(argc!=3){
			printf("Usage: %s -s <mems_file>\n",argv[0]);
			return (-1);
		}
		SortMEMsFile(argv[2]);
		return 0;
	}
	if( ParseArgument(argc,argv,"V",0) ){ // Create MEMs image
		if(argc!=4){
			printf("Usage: %s -v <reference_file> <mems_file>\n",argv[0]);
			return (-1);
		}
		//CreateMemMapImage(argv[2],argv[3]);
		return 0;
	}
	mode=0;
	//if( ParseArgument(argc,argv,"M",0) ) mode=0; // MEMs mode
	argNoNs=ParseArgument(argc,argv,"N",0);
	outFileArgNum=(-1);
	numSeqsInFirstFile=0;
	numFiles=0;
	numSequences=0; // initialize global variable needed by sequence functions
	for(i=1;i<argc;i++){
		if(argv[i][0]=='-'){ // skip arguments for options
			if(argv[i][1]=='L' || argv[i][1]=='l') i++; // skip value of option "-l"
			if(argv[i][1]=='O' || argv[i][1]=='o') i++; // skip value of option "-o"
			continue;
		}
		n = LoadSequencesFromFile( argv[i] , ((numFiles==0)?1:0) , argNoNs ); // load first seq of reference file only
		if(n!=0) numFiles++;
		if(numFiles==1){ // reference file
			outFileArgNum=i;
			numSeqsInFirstFile=n;
		}
	}
	if(numFiles==0) exitMessage("No reference or query files provided");
	if(numFiles==1) exitMessage("No query files provided");
	n=(numSequences-numSeqsInFirstFile);
	if(n==0) exitMessage("No valid query sequences found");
	printf("> %d reference%s and %d quer%s successfully loaded\n",numSeqsInFirstFile,((numSeqsInFirstFile==1)?"":"s"),n,((n==1)?"y":"ies"));
	argBothStrands=ParseArgument(argc,argv,"B",0);
	argMinMemSize=ParseArgument(argc,argv,"L",1);
	if(argMinMemSize==(-1)) argMinMemSize=50; // default minimum MEM length is 50
	n=ParseArgument(argc,argv,"O",2);
	if(n!=(-1)) outFileArgNum=n; // default output base filename is the ref filename
	outFilename=AppendToBasename(argv[outFileArgNum],"-mems.txt");
	switch(mode){
		case 0:
			GetMEMs(numSeqsInFirstFile,numSequences,argMinMemSize,argBothStrands,outFilename);
			break;
		default:
			exitMessage("Invalid arguments");
			break;
	}
	free(outFilename);
	DeleteAllSequences();
	printf("> Done!\n");
	if(PAUSE_AT_EXIT) getchar();
	return 0;
}
