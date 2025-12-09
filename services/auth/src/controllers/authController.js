const User = require('../models/User');
const jwt = require('jsonwebtoken');
const { registerValidation, loginValidation } = require('../middleware/validation');

// Generate JWT Access Token (Short lived: e.g., 15 mins)
const generateAccessToken = (id) => {
  return jwt.sign({ id }, process.env.JWT_SECRET, {
    expiresIn: '15m', 
  });
};

// Generate Refresh Token (Long lived: e.g., 30 days)
const generateRefreshToken = (id) => {
  return jwt.sign({ id }, process.env.JWT_REFRESH_SECRET, {
    expiresIn: '30d',
  });
};

// @desc    Register new user
// @route   POST /api/auth/register
// @access  Public
const registerUser = async (req, res) => {
  // Validate input
  const { error } = registerValidation(req.body);
  if (error) return res.status(400).json({ message: error.details[0].message });

  const { username, email, password } = req.body;

  try {
    // Check if user exists
    const userExists = await User.findOne({ email });
    if (userExists) {
      return res.status(400).json({ message: 'User already exists' });
    }

    // Create user
    const user = await User.create({
      username,
      email,
      password
    });

    if (user) {
      const accessToken = generateAccessToken(user._id);
      const refreshToken = generateRefreshToken(user._id);

      // Save refresh token to DB
      user.refreshToken = refreshToken;
      await user.save();

      res.status(201).json({
        _id: user._id,
        username: user.username,
        email: user.email,
        accessToken,
        refreshToken
      });
    } else {
      res.status(400).json({ message: 'Invalid user data' });
    }
  } catch (error) {
    console.error(error);
    res.status(500).json({ message: 'Server error' });
  }
};

// @desc    Authenticate a user
// @route   POST /api/auth/login
// @access  Public
const loginUser = async (req, res) => {
  // Validate input
  const { error } = loginValidation(req.body);
  if (error) return res.status(400).json({ message: error.details[0].message });

  const { email, password } = req.body;

  try {
    const user = await User.findOne({ email });

    if (user && (await user.matchPassword(password))) {
      const accessToken = generateAccessToken(user._id);
      const refreshToken = generateRefreshToken(user._id);

      // Update refresh token in DB
      user.refreshToken = refreshToken;
      await user.save();

      res.json({
        _id: user._id,
        username: user.username,
        email: user.email,
        accessToken,
        refreshToken
      });
    } else {
      res.status(401).json({ message: 'Invalid email or password' });
    }
  } catch (error) {
    console.error(error);
    res.status(500).json({ message: 'Server error' });
  }
};

// @desc    Refresh Access Token
// @route   POST /api/auth/refresh
// @access  Public
const refreshToken = async (req, res) => {
  const { token } = req.body;

  if (!token) return res.status(401).json({ message: 'No token provided' });

  try {
    // Verify refresh token
    const decoded = jwt.verify(token, process.env.JWT_REFRESH_SECRET);

    // Find user with this id AND this specific refresh token (security check)
    const user = await User.findOne({ _id: decoded.id, refreshToken: token });

    if (!user) {
      return res.status(403).json({ message: 'Invalid refresh token' });
    }

    // Issue new tokens
    const newAccessToken = generateAccessToken(user._id);
    // Optional: Rotate refresh token here for extra security (we'll keep it simple for now)

    res.json({ accessToken: newAccessToken });

  } catch (error) {
    return res.status(403).json({ message: 'Invalid refresh token' });
  }
};

// @desc    Logout user / Invalidate Refresh Token
// @route   POST /api/auth/logout
// @access  Public (or Protected)
const logoutUser = async (req, res) => {
  const { token } = req.body; // The Refresh Token to invalidate

  if (!token) return res.status(200).json({ message: 'Logged out' }); // Already treated as out

  try {
    // Find user by refresh token and remove it
    const user = await User.findOne({ refreshToken: token });
    if (user) {
        user.refreshToken = null;
        await user.save();
    }
    res.status(200).json({ message: 'Logged out successfully' });
  } catch (error) {
    res.status(500).json({ message: 'Server error' });
  }
};

// @desc    Get user data
// @route   GET /api/auth/me
// @access  Private
const getMe = async (req, res) => {
  res.status(200).json(req.user);
};

// @desc    Google OAuth Callback
// @route   GET /api/auth/google/callback
// @access  Public
const googleCallback = async (req, res) => {
  try {
    const user = req.user; // Passport attaches this

    const accessToken = generateAccessToken(user._id);
    const refreshToken = generateRefreshToken(user._id);

    // Update refresh token in DB
    user.refreshToken = refreshToken;
    await user.save();

    // Pass the Google Access Token to the client
    // WARNING: In production, encryption is recommended for this handoff.
    const googleToken = user.googleAccessToken;

    // Redirect to local success page (or deep link)
    res.redirect(`http://localhost:5000/auth/success?accessToken=${accessToken}&refreshToken=${refreshToken}&googleToken=${googleToken}`);

  } catch (error) {
    console.error(error);
    res.status(500).json({ message: 'Server error during Google Auth' });
  }
};

module.exports = {
  registerUser,
  loginUser,
  refreshToken,
  logoutUser,
  getMe,
  googleCallback
};
