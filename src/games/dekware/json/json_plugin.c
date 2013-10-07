


void define_contexts( void )
{
	json_context *context = json_create_context();
	json_context_object *entity = json_create_object( context, sizeof( ENTITY ) );
   json_add_object_member_object( entity, "created_by", offsetof( ENTITY, pCreatedBy ), JSON_Element_ObjectPointer, entity );
   json_add_object_member_object( entity, "within", offsetof( ENTITY, pWithin ), JSON_Element_ObjectPointer, entity );
   json_add_object_member_object( entity, "attached", offsetof( ENTITY, pAttached ), JSON_Element_ObjectPointer, entity );
	json_add_object_member( entity, "flags", offsetof( ENTITY, flags ), JSON_Element_Integer_32, 0 );
   json_add_object_member( entity, "name", offsetof( ENTITY, pName ), JSON_Element_Text, 0 );
}
