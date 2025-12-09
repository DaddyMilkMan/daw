const express = require('express');
const router = express.Router();
const multer = require('multer');
const path = require('path');
const fs = require('fs');
const { protect } = require('../middleware/authMiddleware');
const Project = require('../models/Project');

// Configure storage
const storage = multer.diskStorage({
  destination: function (req, file, cb) {
    const uploadDir = 'uploads/';
    // Ensure dir exists
    if (!fs.existsSync(uploadDir)){
        fs.mkdirSync(uploadDir);
    }
    cb(null, uploadDir);
  },
  filename: function (req, file, cb) {
    // Unique filename: project-USERID-TIMESTAMP.zenith
    const uniqueSuffix = Date.now() + '-' + Math.round(Math.random() * 1E9);
    cb(null, file.fieldname + '-' + req.user._id + '-' + uniqueSuffix + path.extname(file.originalname));
  }
});

const upload = multer({ 
    storage: storage,
    limits: { fileSize: 50 * 1024 * 1024 } // 50MB limit
});

// @desc    Upload a project file
// @route   POST /api/projects/upload
// @access  Private
router.post('/upload', protect, upload.single('projectFile'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ message: 'No file uploaded' });
        }

        const { name, description, isPublic } = req.body;

        const project = await Project.create({
            owner: req.user._id,
            name: name || req.file.originalname,
            description: description || '',
            filename: req.file.filename,
            path: req.file.path,
            size: req.file.size,
            isPublic: isPublic === 'true'
        });

        res.status(201).json(project);
    } catch (error) {
        console.error(error);
        res.status(500).json({ message: 'Server error during upload' });
    }
});

// @desc    Get user's projects
// @route   GET /api/projects
// @access  Private
router.get('/', protect, async (req, res) => {
    try {
        const projects = await Project.find({ owner: req.user._id }).sort({ updatedAt: -1 });
        res.json(projects);
    } catch (error) {
        res.status(500).json({ message: 'Server error' });
    }
});

module.exports = router;
