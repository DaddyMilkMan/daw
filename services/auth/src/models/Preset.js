const mongoose = require('mongoose');

const presetSchema = new mongoose.Schema({
    owner: {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'User',
        required: true
    },
    name: {
        type: String,
        required: true,
        trim: true,
        index: true
    },
    description: {
        type: String,
        default: ''
    },
    // Which plugin/instrument this preset is for
    pluginId: {
        type: String,
        required: true,
        index: true
    },
    pluginName: {
        type: String,
        required: true
    },
    // The preset file data
    filename: {
        type: String,
        required: true
    },
    path: {
        type: String,
        required: true
    },
    size: {
        type: Number
    },
    // Sharing settings
    isPublic: {
        type: Boolean,
        default: false,
        index: true
    },
    // Categorization
    category: {
        type: String,
        enum: ['synth', 'bass', 'pad', 'lead', 'keys', 'fx', 'drums', 'other'],
        default: 'other'
    },
    tags: [{
        type: String,
        trim: true
    }],
    // Statistics
    downloads: {
        type: Number,
        default: 0
    },
    likes: {
        type: Number,
        default: 0
    },
    likedBy: [{
        type: mongoose.Schema.Types.ObjectId,
        ref: 'User'
    }],
    // Preview audio (optional)
    previewUrl: {
        type: String
    },
    // Timestamps
    createdAt: {
        type: Date,
        default: Date.now,
        index: true
    },
    updatedAt: {
        type: Date,
        default: Date.now
    }
});

// Create compound index for efficient public preset queries
presetSchema.index({ isPublic: 1, createdAt: -1 });
presetSchema.index({ isPublic: 1, downloads: -1 });
presetSchema.index({ pluginId: 1, isPublic: 1 });

// Update timestamp on save
presetSchema.pre('save', function (next) {
    this.updatedAt = Date.now();
    next();
});

const Preset = mongoose.model('Preset', presetSchema);

module.exports = Preset;
