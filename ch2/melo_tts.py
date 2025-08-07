from melo.api import TTS

# Speed is adjustable
speed = 1.0

# CPU is sufficient for real-time inference.
# You can set it manually to 'cpu' or 'cuda' or 'cuda:0' or 'mps'
device = 'auto' # Will automatically use GPU if available

# English
text = "signal processing"
model = TTS(language='EN', device=device)
speaker_ids = model.hps.data.spk2id

output_path = 'en.wav'
model.tts_to_file(text, speaker_ids['EN-Default'], output_path, speed=speed)
