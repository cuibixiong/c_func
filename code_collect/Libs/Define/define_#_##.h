#define FACTORY_CREATE(name) \
static sp<MediaSource> Make##name(const sp<MediaSource> &source) { \ 
     return new name(source); \
}
#define FACTORY_CREATE_ENCODER(name) \
static sp<MediaSource> Make##name(const sp<MediaSource> &source, const sp<MetaData> &meta) { \ 
    return new name(source, meta); \
}
#define FACTORY_REF(name) 
{#name,Make##name},
FACTORY_CREATE(MP3Decoder)
FACTORY_CREATE(AMRNBDecoder)
FACTORY_CREATE(AMRWBDecoder)
FACTORY_CREATE(AACDecoder)
FACTORY_CREATE(AVCDecoder)
FACTORY_CREATE(G711Decoder)
FACTORY_CREATE(M4vH263Decoder)
FACTORY_CREATE(VorbisDecoder)
FACTORY_CREATE(VPXDecoder)
FACTORY_CREATE_ENCODER(AMRNBEncoder)
FACTORY_CREATE_ENCODER(AMRWBEncoder)
FACTORY_CREATE_ENCODER(AACEncoder)
FACTORY_CREATE_ENCODER(AVCEncoder)
FACTORY_CREATE_ENCODER(M4vH263Encoder)
