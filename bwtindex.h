unsigned int FMI_PositionInText( unsigned int bwtpos );
unsigned int FMI_FollowLetter( char c , unsigned int *topPointer , unsigned int *bottomPointer );
unsigned int FMI_LeftJump( unsigned int bwtpos );
char FMI_GetCharAtBWTPos( unsigned int bwtpos );
void FMI_GetCharCountsAtBWTInterval( unsigned int topPtr , unsigned int bottomPtr , int *counts );
void FMI_FreeIndex();
void FMI_BuildIndex(char **inputTexts, unsigned int *inputTextSizes, unsigned int inputNumTexts, unsigned char **lcpArrayPointer, char verbose);
unsigned int FMI_GetTextSize();
unsigned int FMI_GetBWTSize();
char *FMI_GetTextFilename();
