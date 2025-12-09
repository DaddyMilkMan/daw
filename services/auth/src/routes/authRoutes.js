const express = require('express');
const passport = require('passport');
const router = express.Router();
const {
  registerUser,
  loginUser,
  getMe,
  refreshToken,
  logoutUser,
  googleCallback
} = require('../controllers/authController');
const { protect } = require('../middleware/authMiddleware');

router.post('/register', registerUser);
router.post('/login', loginUser);
router.post('/refresh', refreshToken);
router.post('/logout', logoutUser);
router.get('/me', protect, getMe);

// Google OAuth
router.get('/google', passport.authenticate('google', { scope: ['profile', 'email'] }));
router.get('/google/callback', 
  passport.authenticate('google', { session: false, failureRedirect: '/login' }),
  googleCallback
);

module.exports = router;
