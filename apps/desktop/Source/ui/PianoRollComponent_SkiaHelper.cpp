
SkColor PianoRollComponent::getSkiaColorForVelocity(int velocity) const
{
    juce::Colour c = getColorForVelocity(velocity);
    return SkColorSetARGB(c.getAlpha(), c.getRed(), c.getGreen(), c.getBlue());
}
