const GoogleStrategy = require('passport-google-oauth20').Strategy;
const User = require('../models/User');

module.exports = function(passport) {
  passport.use(new GoogleStrategy({
    clientID: process.env.GOOGLE_CLIENT_ID,
    clientSecret: process.env.GOOGLE_CLIENT_SECRET,
    callbackURL: process.env.GOOGLE_CALLBACK_URL || '/api/auth/google/callback'
  },
  async (accessToken, refreshToken, profile, done) => {
    try {
      // 1. Check if user exists with this Google ID
      let user = await User.findOne({ googleId: profile.id });

      if (user) {
        return done(null, user);
      }

      // 2. Check if user exists with this email (link account)
      // Note: Google emails are verified, so this is generally safe.
      const email = profile.emails[0].value;
      user = await User.findOne({ email: email });

      if (user) {
        // Link Google ID to existing account
        user.googleId = profile.id;
        await user.save();
        return done(null, user);
      }

      // 3. Create new user
      const newUser = {
        googleId: profile.id,
        email: email,
        username: profile.displayName || email.split('@')[0],
        // No password needed
      };

      user = await User.create(newUser);
      done(null, user);

    } catch (err) {
      console.error(err);
      done(err, null);
    }
  }));

  // Serialization (if using session cookies, but we use JWTs so this is minimal)
  passport.serializeUser((user, done) => {
    done(null, user.id);
  });

  passport.deserializeUser((id, done) => {
    User.findById(id, (err, user) => done(err, user));
  });
};
