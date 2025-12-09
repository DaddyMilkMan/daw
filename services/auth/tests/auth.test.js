const request = require('supertest');
const app = require('../src/server');
const User = require('../src/models/User');
const mongoose = require('mongoose');
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');

// Mocking Mongoose User model functions
jest.mock('../src/models/User');

// Mock env variables
process.env.JWT_SECRET = 'test_secret';
process.env.JWT_REFRESH_SECRET = 'test_refresh_secret';

describe('Auth API', () => {
  
  beforeEach(() => {
    jest.clearAllMocks();
  });

  // --- Registration Tests ---
  describe('POST /api/auth/register', () => {
    it('should register a new user', async () => {
      User.findOne.mockResolvedValue(null);
      User.create.mockResolvedValue({
        _id: '123',
        username: 'testuser',
        email: 'test@example.com',
        save: jest.fn()
      });

      const res = await request(app)
        .post('/api/auth/register')
        .send({
          username: 'testuser',
          email: 'test@example.com',
          password: 'password123'
        });

      expect(res.statusCode).toBe(201);
      expect(res.body).toHaveProperty('accessToken');
      expect(res.body).toHaveProperty('refreshToken');
    });

    it('should return 400 if user already exists', async () => {
      User.findOne.mockResolvedValue({ email: 'test@example.com' });

      const res = await request(app)
        .post('/api/auth/register')
        .send({
          username: 'testuser',
          email: 'test@example.com',
          password: 'password123'
        });

      expect(res.statusCode).toBe(400);
      expect(res.body.message).toBe('User already exists');
    });

    it('should return 400 for invalid input', async () => {
      const res = await request(app)
        .post('/api/auth/register')
        .send({
          username: 'ab', // too short
          email: 'invalid-email',
          password: '123'
        });

      expect(res.statusCode).toBe(400);
    });
  });

  // --- Login Tests ---
  describe('POST /api/auth/login', () => {
    it('should login valid user', async () => {
      const mockUser = {
        _id: '123',
        username: 'testuser',
        email: 'test@example.com',
        matchPassword: jest.fn().mockResolvedValue(true),
        save: jest.fn()
      };
      User.findOne.mockResolvedValue(mockUser);

      const res = await request(app)
        .post('/api/auth/login')
        .send({
          email: 'test@example.com',
          password: 'password123'
        });

      expect(res.statusCode).toBe(200);
      expect(res.body).toHaveProperty('accessToken');
    });

    it('should fail with invalid credentials', async () => {
      const mockUser = {
        matchPassword: jest.fn().mockResolvedValue(false)
      };
      User.findOne.mockResolvedValue(mockUser);

      const res = await request(app)
        .post('/api/auth/login')
        .send({
          email: 'test@example.com',
          password: 'wrongpassword'
        });

      expect(res.statusCode).toBe(401);
    });
  });

  // --- Refresh Token Tests ---
  describe('POST /api/auth/refresh', () => {
    it('should refresh access token', async () => {
      const token = jwt.sign({ id: '123' }, process.env.JWT_REFRESH_SECRET);
      User.findOne.mockResolvedValue({ _id: '123' });

      const res = await request(app)
        .post('/api/auth/refresh')
        .send({ token });

      expect(res.statusCode).toBe(200);
      expect(res.body).toHaveProperty('accessToken');
    });

    it('should reject invalid refresh token', async () => {
      const res = await request(app)
        .post('/api/auth/refresh')
        .send({ token: 'invalid_token' });

      expect(res.statusCode).toBe(403);
    });
  });

  // --- Protected Route Tests ---
  describe('GET /api/auth/me', () => {
    it('should return user profile for valid token', async () => {
      const token = jwt.sign({ id: '123' }, process.env.JWT_SECRET);
      User.findById.mockReturnValue({
        select: jest.fn().mockResolvedValue({ _id: '123', username: 'testuser' })
      });

      const res = await request(app)
        .get('/api/auth/me')
        .set('Authorization', `Bearer ${token}`);

      expect(res.statusCode).toBe(200);
      expect(res.body.username).toBe('testuser');
    });

    it('should reject without token', async () => {
      const res = await request(app).get('/api/auth/me');
      expect(res.statusCode).toBe(401);
    });
  });
});
