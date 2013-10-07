
typedef struct editor_tag {
	char byPrivateData;
} *PEDITOR;

PEDITOR CreateEditor( POBJECT po );
void DestroyEditor( PEDITOR pe );

