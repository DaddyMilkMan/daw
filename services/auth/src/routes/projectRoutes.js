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
        if (!fs.existsSync(uploadDir)) {
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

// @desc    Save a new version/snapshot of a project
// @route   POST /api/projects/:id/versions
// @access  Private
router.post('/:id/versions', protect, upload.single('projectFile'), async (req, res) => {
    try {
        const project = await Project.findById(req.params.id);

        if (!project) {
            return res.status(404).json({ message: 'Project not found' });
        }

        // Verify ownership
        if (project.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized to modify this project' });
        }

        if (!req.file) {
            return res.status(400).json({ message: 'No project file provided for version' });
        }

        const { label, changelog } = req.body;

        // Initialize versions array if it doesn't exist
        if (!project.versions) {
            project.versions = [];
        }

        // Create version entry
        const versionNumber = project.versions.length + 1;
        const version = {
            versionNumber,
            label: label || `Version ${versionNumber}`,
            changelog: changelog || '',
            filename: req.file.filename,
            path: req.file.path,
            size: req.file.size,
            createdAt: new Date()
        };

        project.versions.push(version);
        project.updatedAt = new Date();

        // Update main project file reference to latest version
        project.filename = req.file.filename;
        project.path = req.file.path;
        project.size = req.file.size;

        await project.save();

        res.status(201).json({
            message: 'Version saved successfully',
            version,
            totalVersions: project.versions.length
        });
    } catch (error) {
        console.error('Error saving project version:', error);
        res.status(500).json({ message: 'Server error saving version' });
    }
});

// @desc    Get all versions of a project
// @route   GET /api/projects/:id/versions
// @access  Private
router.get('/:id/versions', protect, async (req, res) => {
    try {
        const project = await Project.findById(req.params.id);

        if (!project) {
            return res.status(404).json({ message: 'Project not found' });
        }

        // Verify ownership
        if (project.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized to view this project' });
        }

        res.json({
            projectId: project._id,
            projectName: project.name,
            versions: project.versions || [],
            currentVersion: (project.versions || []).length
        });
    } catch (error) {
        console.error('Error fetching project versions:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

// @desc    Restore a specific version
// @route   POST /api/projects/:id/versions/:versionNumber/restore
// @access  Private
router.post('/:id/versions/:versionNumber/restore', protect, async (req, res) => {
    try {
        const project = await Project.findById(req.params.id);

        if (!project) {
            return res.status(404).json({ message: 'Project not found' });
        }

        if (project.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized to modify this project' });
        }

        const versionNumber = parseInt(req.params.versionNumber);
        const version = project.versions?.find(v => v.versionNumber === versionNumber);

        if (!version) {
            return res.status(404).json({ message: 'Version not found' });
        }

        // Check if version file still exists
        if (!fs.existsSync(version.path)) {
            return res.status(404).json({ message: 'Version file no longer exists' });
        }

        // Create a new version from the restore operation
        const restoreVersion = {
            versionNumber: (project.versions?.length || 0) + 1,
            label: `Restored from ${version.label}`,
            changelog: `Restored to version ${versionNumber}`,
            filename: version.filename,
            path: version.path,
            size: version.size,
            createdAt: new Date()
        };

        project.versions.push(restoreVersion);
        project.filename = version.filename;
        project.path = version.path;
        project.size = version.size;
        project.updatedAt = new Date();

        await project.save();

        res.json({
            message: 'Version restored successfully',
            restoredVersion: versionNumber,
            newVersion: restoreVersion
        });
    } catch (error) {
        console.error('Error restoring project version:', error);
        res.status(500).json({ message: 'Server error restoring version' });
    }
});

// @desc    Download a specific version
// @route   GET /api/projects/:id/versions/:versionNumber/download
// @access  Private
router.get('/:id/versions/:versionNumber/download', protect, async (req, res) => {
    try {
        const project = await Project.findById(req.params.id);

        if (!project) {
            return res.status(404).json({ message: 'Project not found' });
        }

        if (project.owner.toString() !== req.user._id.toString()) {
            return res.status(403).json({ message: 'Not authorized' });
        }

        const versionNumber = parseInt(req.params.versionNumber);
        const version = project.versions?.find(v => v.versionNumber === versionNumber);

        if (!version) {
            return res.status(404).json({ message: 'Version not found' });
        }

        if (!fs.existsSync(version.path)) {
            return res.status(404).json({ message: 'Version file not found' });
        }

        res.download(version.path, `${project.name}_v${versionNumber}.zenith`);
    } catch (error) {
        console.error('Error downloading version:', error);
        res.status(500).json({ message: 'Server error' });
    }
});

module.exports = router;
