import sys

file_path = '/home/micah/Desktop/zenith/daw/apps/desktop/Source/ui/views/SessionViewComponent.cpp'

with open(file_path, 'r') as f:
    lines = f.readlines()

content = "".join(lines)

# Fix drawMidiPreview loop
old_draw = """  for (const auto &note : slot.midiNotes) {
    int pitch = note.first;
    float position = note.second;

    float y =
        contentRect.bottom() - ((pitch - 36) / 60.0f) * contentRect.height();
    float x = contentRect.left() + position * contentRect.width();

    y = std::clamp(y, contentRect.top(), contentRect.bottom());

    canvas->drawRect(SkRect::MakeXYWH(x, y, 8, 4), notePaint);
  }"""

new_draw = """  for (const auto &note : slot.midiNotes) {
    int pitch = note.pitch;
    float position = note.startPos;
    float length = note.length;

    float y =
        contentRect.bottom() - ((pitch - 36) / 60.0f) * contentRect.height();
    float x = contentRect.left() + position * contentRect.width();
    float w = std::max(2.0f, length * contentRect.width());

    y = std::clamp(y, contentRect.top(), contentRect.bottom());

    canvas->drawRect(SkRect::MakeXYWH(x, y, w, 4), notePaint);
  }"""

if old_draw in content:
    content = content.replace(old_draw, new_draw)

# Fix buildMidiPreview loop
old_build = """    float normalizedPos = static_cast<float>(startBeats / clipLength);
    slot.midiNotes.push_back({pitch, normalizedPos});"""

new_build = """    double lengthBeats = static_cast<double>(noteTree[ProjectState::PROP_LENGTH]);
    float normalizedPos = static_cast<float>(startBeats / clipLength);
    float normalizedLen = static_cast<float>(lengthBeats / clipLength);
    slot.midiNotes.push_back({pitch, normalizedPos, normalizedLen});"""

if old_build in content:
    content = content.replace(old_build, new_build)

# Add missing braces
content = content.strip()
brace_diff = content.count('{') - content.count('}')
if brace_diff > 0:
    content += '\n' + ('}' * brace_diff)

if "// namespace zenith" not in content.splitlines()[-1]:
    content += "\n} // namespace zenith"

with open(file_path, 'w') as f:
    f.write(content)
    f.write('\n')

print("Successfully updated SessionViewComponent.cpp")
