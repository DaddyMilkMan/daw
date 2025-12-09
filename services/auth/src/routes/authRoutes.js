const express = require('express');
const router = express.Router();
const {
  registerUser,
  loginUser,
  getMe,
  refreshToken,
  logoutUser
} = require('../controllers/authController');
const { protect } = require('../middleware/authMiddleware');

router.post('/register', registerUser);
router.post('/login', loginUser);
router.post('/refresh', refreshToken);
router.post('/logout', logoutUser);
router.get('/me', protect, getMe);

module.exports = router;
