#ifndef EZPACKER_IDECODERDEFINES_H
#define EZPACKER_IDECODERDEFINES_H

/*
 * File used to store different preprocessor definitions that are of help.
 */

/**
 * This define adds an unsupported instruction by setting its value to Unsupported.
 */
#define UNSUPPORTED_INSTRUCTION_TYPE(Type) Type = DecodedInstructionType::Unsupported

#endif // EZPACKER_IDECODERDEFINES_H
