/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: WhisperTextProtocol.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "WhisperTextProtocol.pb-c.h"
void   textsecure__whisper_message__init
                     (Textsecure__WhisperMessage         *message)
{
  static Textsecure__WhisperMessage init_value = TEXTSECURE__WHISPER_MESSAGE__INIT;
  *message = init_value;
}
size_t textsecure__whisper_message__get_packed_size
                     (const Textsecure__WhisperMessage *message)
{
  assert(message->base.descriptor == &textsecure__whisper_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t textsecure__whisper_message__pack
                     (const Textsecure__WhisperMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &textsecure__whisper_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t textsecure__whisper_message__pack_to_buffer
                     (const Textsecure__WhisperMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &textsecure__whisper_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Textsecure__WhisperMessage *
       textsecure__whisper_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Textsecure__WhisperMessage *)
     protobuf_c_message_unpack (&textsecure__whisper_message__descriptor,
                                allocator, len, data);
}
void   textsecure__whisper_message__free_unpacked
                     (Textsecure__WhisperMessage *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &textsecure__whisper_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   textsecure__pre_key_whisper_message__init
                     (Textsecure__PreKeyWhisperMessage         *message)
{
  static Textsecure__PreKeyWhisperMessage init_value = TEXTSECURE__PRE_KEY_WHISPER_MESSAGE__INIT;
  *message = init_value;
}
size_t textsecure__pre_key_whisper_message__get_packed_size
                     (const Textsecure__PreKeyWhisperMessage *message)
{
  assert(message->base.descriptor == &textsecure__pre_key_whisper_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t textsecure__pre_key_whisper_message__pack
                     (const Textsecure__PreKeyWhisperMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &textsecure__pre_key_whisper_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t textsecure__pre_key_whisper_message__pack_to_buffer
                     (const Textsecure__PreKeyWhisperMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &textsecure__pre_key_whisper_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Textsecure__PreKeyWhisperMessage *
       textsecure__pre_key_whisper_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Textsecure__PreKeyWhisperMessage *)
     protobuf_c_message_unpack (&textsecure__pre_key_whisper_message__descriptor,
                                allocator, len, data);
}
void   textsecure__pre_key_whisper_message__free_unpacked
                     (Textsecure__PreKeyWhisperMessage *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &textsecure__pre_key_whisper_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   textsecure__key_exchange_message__init
                     (Textsecure__KeyExchangeMessage         *message)
{
  static Textsecure__KeyExchangeMessage init_value = TEXTSECURE__KEY_EXCHANGE_MESSAGE__INIT;
  *message = init_value;
}
size_t textsecure__key_exchange_message__get_packed_size
                     (const Textsecure__KeyExchangeMessage *message)
{
  assert(message->base.descriptor == &textsecure__key_exchange_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t textsecure__key_exchange_message__pack
                     (const Textsecure__KeyExchangeMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &textsecure__key_exchange_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t textsecure__key_exchange_message__pack_to_buffer
                     (const Textsecure__KeyExchangeMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &textsecure__key_exchange_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Textsecure__KeyExchangeMessage *
       textsecure__key_exchange_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Textsecure__KeyExchangeMessage *)
     protobuf_c_message_unpack (&textsecure__key_exchange_message__descriptor,
                                allocator, len, data);
}
void   textsecure__key_exchange_message__free_unpacked
                     (Textsecure__KeyExchangeMessage *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &textsecure__key_exchange_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   textsecure__sender_key_message__init
                     (Textsecure__SenderKeyMessage         *message)
{
  static Textsecure__SenderKeyMessage init_value = TEXTSECURE__SENDER_KEY_MESSAGE__INIT;
  *message = init_value;
}
size_t textsecure__sender_key_message__get_packed_size
                     (const Textsecure__SenderKeyMessage *message)
{
  assert(message->base.descriptor == &textsecure__sender_key_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t textsecure__sender_key_message__pack
                     (const Textsecure__SenderKeyMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &textsecure__sender_key_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t textsecure__sender_key_message__pack_to_buffer
                     (const Textsecure__SenderKeyMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &textsecure__sender_key_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Textsecure__SenderKeyMessage *
       textsecure__sender_key_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Textsecure__SenderKeyMessage *)
     protobuf_c_message_unpack (&textsecure__sender_key_message__descriptor,
                                allocator, len, data);
}
void   textsecure__sender_key_message__free_unpacked
                     (Textsecure__SenderKeyMessage *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &textsecure__sender_key_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   textsecure__sender_key_distribution_message__init
                     (Textsecure__SenderKeyDistributionMessage         *message)
{
  static Textsecure__SenderKeyDistributionMessage init_value = TEXTSECURE__SENDER_KEY_DISTRIBUTION_MESSAGE__INIT;
  *message = init_value;
}
size_t textsecure__sender_key_distribution_message__get_packed_size
                     (const Textsecure__SenderKeyDistributionMessage *message)
{
  assert(message->base.descriptor == &textsecure__sender_key_distribution_message__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t textsecure__sender_key_distribution_message__pack
                     (const Textsecure__SenderKeyDistributionMessage *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &textsecure__sender_key_distribution_message__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t textsecure__sender_key_distribution_message__pack_to_buffer
                     (const Textsecure__SenderKeyDistributionMessage *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &textsecure__sender_key_distribution_message__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Textsecure__SenderKeyDistributionMessage *
       textsecure__sender_key_distribution_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Textsecure__SenderKeyDistributionMessage *)
     protobuf_c_message_unpack (&textsecure__sender_key_distribution_message__descriptor,
                                allocator, len, data);
}
void   textsecure__sender_key_distribution_message__free_unpacked
                     (Textsecure__SenderKeyDistributionMessage *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &textsecure__sender_key_distribution_message__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor textsecure__whisper_message__field_descriptors[4] =
{
  {
    "ratchetKey",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__WhisperMessage, has_ratchetkey),
    offsetof(Textsecure__WhisperMessage, ratchetkey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "counter",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__WhisperMessage, has_counter),
    offsetof(Textsecure__WhisperMessage, counter),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "previousCounter",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__WhisperMessage, has_previouscounter),
    offsetof(Textsecure__WhisperMessage, previouscounter),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ciphertext",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__WhisperMessage, has_ciphertext),
    offsetof(Textsecure__WhisperMessage, ciphertext),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned textsecure__whisper_message__field_indices_by_name[] = {
  3,   /* field[3] = ciphertext */
  1,   /* field[1] = counter */
  2,   /* field[2] = previousCounter */
  0,   /* field[0] = ratchetKey */
};
static const ProtobufCIntRange textsecure__whisper_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor textsecure__whisper_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "textsecure.WhisperMessage",
  "WhisperMessage",
  "Textsecure__WhisperMessage",
  "textsecure",
  sizeof(Textsecure__WhisperMessage),
  4,
  textsecure__whisper_message__field_descriptors,
  textsecure__whisper_message__field_indices_by_name,
  1,  textsecure__whisper_message__number_ranges,
  (ProtobufCMessageInit) textsecure__whisper_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor textsecure__pre_key_whisper_message__field_descriptors[6] =
{
  {
    "preKeyId",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__PreKeyWhisperMessage, has_prekeyid),
    offsetof(Textsecure__PreKeyWhisperMessage, prekeyid),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "baseKey",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__PreKeyWhisperMessage, has_basekey),
    offsetof(Textsecure__PreKeyWhisperMessage, basekey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "identityKey",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__PreKeyWhisperMessage, has_identitykey),
    offsetof(Textsecure__PreKeyWhisperMessage, identitykey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "message",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__PreKeyWhisperMessage, has_message),
    offsetof(Textsecure__PreKeyWhisperMessage, message),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "registrationId",
    5,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__PreKeyWhisperMessage, has_registrationid),
    offsetof(Textsecure__PreKeyWhisperMessage, registrationid),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "signedPreKeyId",
    6,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__PreKeyWhisperMessage, has_signedprekeyid),
    offsetof(Textsecure__PreKeyWhisperMessage, signedprekeyid),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned textsecure__pre_key_whisper_message__field_indices_by_name[] = {
  1,   /* field[1] = baseKey */
  2,   /* field[2] = identityKey */
  3,   /* field[3] = message */
  0,   /* field[0] = preKeyId */
  4,   /* field[4] = registrationId */
  5,   /* field[5] = signedPreKeyId */
};
static const ProtobufCIntRange textsecure__pre_key_whisper_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 6 }
};
const ProtobufCMessageDescriptor textsecure__pre_key_whisper_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "textsecure.PreKeyWhisperMessage",
  "PreKeyWhisperMessage",
  "Textsecure__PreKeyWhisperMessage",
  "textsecure",
  sizeof(Textsecure__PreKeyWhisperMessage),
  6,
  textsecure__pre_key_whisper_message__field_descriptors,
  textsecure__pre_key_whisper_message__field_indices_by_name,
  1,  textsecure__pre_key_whisper_message__number_ranges,
  (ProtobufCMessageInit) textsecure__pre_key_whisper_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor textsecure__key_exchange_message__field_descriptors[5] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__KeyExchangeMessage, has_id),
    offsetof(Textsecure__KeyExchangeMessage, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "baseKey",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__KeyExchangeMessage, has_basekey),
    offsetof(Textsecure__KeyExchangeMessage, basekey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ratchetKey",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__KeyExchangeMessage, has_ratchetkey),
    offsetof(Textsecure__KeyExchangeMessage, ratchetkey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "identityKey",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__KeyExchangeMessage, has_identitykey),
    offsetof(Textsecure__KeyExchangeMessage, identitykey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "baseKeySignature",
    5,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__KeyExchangeMessage, has_basekeysignature),
    offsetof(Textsecure__KeyExchangeMessage, basekeysignature),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned textsecure__key_exchange_message__field_indices_by_name[] = {
  1,   /* field[1] = baseKey */
  4,   /* field[4] = baseKeySignature */
  0,   /* field[0] = id */
  3,   /* field[3] = identityKey */
  2,   /* field[2] = ratchetKey */
};
static const ProtobufCIntRange textsecure__key_exchange_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 5 }
};
const ProtobufCMessageDescriptor textsecure__key_exchange_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "textsecure.KeyExchangeMessage",
  "KeyExchangeMessage",
  "Textsecure__KeyExchangeMessage",
  "textsecure",
  sizeof(Textsecure__KeyExchangeMessage),
  5,
  textsecure__key_exchange_message__field_descriptors,
  textsecure__key_exchange_message__field_indices_by_name,
  1,  textsecure__key_exchange_message__number_ranges,
  (ProtobufCMessageInit) textsecure__key_exchange_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor textsecure__sender_key_message__field_descriptors[3] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__SenderKeyMessage, has_id),
    offsetof(Textsecure__SenderKeyMessage, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "iteration",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__SenderKeyMessage, has_iteration),
    offsetof(Textsecure__SenderKeyMessage, iteration),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ciphertext",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__SenderKeyMessage, has_ciphertext),
    offsetof(Textsecure__SenderKeyMessage, ciphertext),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned textsecure__sender_key_message__field_indices_by_name[] = {
  2,   /* field[2] = ciphertext */
  0,   /* field[0] = id */
  1,   /* field[1] = iteration */
};
static const ProtobufCIntRange textsecure__sender_key_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor textsecure__sender_key_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "textsecure.SenderKeyMessage",
  "SenderKeyMessage",
  "Textsecure__SenderKeyMessage",
  "textsecure",
  sizeof(Textsecure__SenderKeyMessage),
  3,
  textsecure__sender_key_message__field_descriptors,
  textsecure__sender_key_message__field_indices_by_name,
  1,  textsecure__sender_key_message__number_ranges,
  (ProtobufCMessageInit) textsecure__sender_key_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor textsecure__sender_key_distribution_message__field_descriptors[4] =
{
  {
    "id",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__SenderKeyDistributionMessage, has_id),
    offsetof(Textsecure__SenderKeyDistributionMessage, id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "iteration",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_UINT32,
    offsetof(Textsecure__SenderKeyDistributionMessage, has_iteration),
    offsetof(Textsecure__SenderKeyDistributionMessage, iteration),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "chainKey",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__SenderKeyDistributionMessage, has_chainkey),
    offsetof(Textsecure__SenderKeyDistributionMessage, chainkey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "signingKey",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_BYTES,
    offsetof(Textsecure__SenderKeyDistributionMessage, has_signingkey),
    offsetof(Textsecure__SenderKeyDistributionMessage, signingkey),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned textsecure__sender_key_distribution_message__field_indices_by_name[] = {
  2,   /* field[2] = chainKey */
  0,   /* field[0] = id */
  1,   /* field[1] = iteration */
  3,   /* field[3] = signingKey */
};
static const ProtobufCIntRange textsecure__sender_key_distribution_message__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor textsecure__sender_key_distribution_message__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "textsecure.SenderKeyDistributionMessage",
  "SenderKeyDistributionMessage",
  "Textsecure__SenderKeyDistributionMessage",
  "textsecure",
  sizeof(Textsecure__SenderKeyDistributionMessage),
  4,
  textsecure__sender_key_distribution_message__field_descriptors,
  textsecure__sender_key_distribution_message__field_indices_by_name,
  1,  textsecure__sender_key_distribution_message__number_ranges,
  (ProtobufCMessageInit) textsecure__sender_key_distribution_message__init,
  NULL,NULL,NULL    /* reserved[123] */
};
