# StudioOS Media

Media processing backend built around gRPC, FFmpeg, and OpenCV.

## Storage

StudioOS Media is prepared to use AWS S3-compatible object storage for uploads and processed outputs.
Configure the bucket details in `config/config.json`; credentials should come from the normal AWS chain such as environment variables, `aws configure`, or an IAM role.

Example object reference:

```text
s3://studioos-media/uploads/song.wav
```

## Milestones

- Milestone 1 - Core media processing
- Milestone 2 - Streaming output
- Milestone 3 - Service API
- Milestone 4 - Automation and configuration
- Milestone 5 - Advertisements

### Milestone 5 - Advertisements

AdService

- [ ] Video ads
- [ ] Image ads
- [ ] Audio ads
- [ ] Scheduling
- [ ] Impression tracking
- [ ] Click tracking
