/* Header file defining specific edit interface methods. These
   work on the generic PSI_CONTROL that is a button, but do
   expect specifically a control that is their type.             */

PSI_EDIT_NAMESPACE

// for convenience of migration, return pc
PSI_PROC( PSI_CONTROL, SetEditControlReadOnly )( PSI_CONTROL pc, LOGICAL bEnable );

PSI_EDIT_NAMESPACE_END
USE_PSI_EDIT_NAMESPACE
