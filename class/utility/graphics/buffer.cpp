#ifndef _BUFFER_CLASS
#include <chrono>

#include "./buffer.h"
#include "buffer.h"

Buffer::Buffer(bool initialize)
{
    if (!initialize) return;
    this->initialized = true;

    glGenBuffers(1, &this->buffer_id);

    glNamedBufferData(this->buffer_id, 0, NULL, GL_DYNAMIC_DRAW);
}
void Buffer::dispose() {
    glDeleteBuffers(1, &this->buffer_id);
}
bool Buffer::is_buffer() {
    if (!this->initialized) return false;
    return glIsBuffer(this->buffer_id);
}

void Buffer::set_data(GLsizeiptr size, const void *data) {
    this->buffer_size = size;
    glNamedBufferData(this->buffer_id, size, data, GL_DYNAMIC_DRAW);
}
void Buffer::set_data(GLsizeiptr offset, GLsizeiptr size, const void *data) {
    glNamedBufferSubData(this->buffer_id, offset, size, data);
}

void Buffer::bind_buffer(GLuint index) {
    this->binding = index;
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, this->buffer_id);
}
unsigned int Buffer::get_used_size() {
    return this->buffer_size;
}


GrowableBuffer::GrowableBuffer(bool initialize, unsigned int added_storage, unsigned int start_size): Buffer(initialize)
{
    if (!initialize) return;
    this->buffer_added_storage = added_storage;
    this->used_storage = 0;

    this->buffer_size = start_size;
    glNamedBufferData(this->buffer_id, this->buffer_size, NULL, GL_DYNAMIC_DRAW);

}
void GrowableBuffer::set_data(GLsizeiptr size, const void *data) {
    if (size > this->buffer_size + this->buffer_added_storage) {
        std::cerr << "too big !!";
        exit(-1);
    }
    if (size < this->buffer_size && size > this->buffer_size - 2*this->buffer_added_storage) { // do not realocate if the size is small enough
        return this->set_data(0, size, data);
    }

    this->used_storage = size;
    this->buffer_size = size + this->buffer_added_storage;
    glNamedBufferData(this->buffer_id, this->buffer_size, NULL, GL_DYNAMIC_DRAW);

    glNamedBufferSubData(this->buffer_id, 0, size, data);
}
void GrowableBuffer::set_data(GLsizeiptr offset, GLsizeiptr size, const void *data) {
    if (offset + size > this->buffer_size + this->buffer_added_storage) {
        std::cerr << "too big !!";
        exit(-1);
    }
    if (offset + size <= this->buffer_size) {
        this->used_storage = __max(this->used_storage, offset + size);
        glNamedBufferSubData(this->buffer_id, offset, size, data);
        return;
    }
    if (offset == 0 && size > this->buffer_size) {
        this->set_data(size, data);
        return;
    }
    
    const void* old_data = glMapNamedBufferRange(this->buffer_id, 0, offset, GL_MAP_READ_BIT);
    glUnmapNamedBuffer(this->buffer_id);

    glNamedBufferData(this->buffer_id, offset + size + this->buffer_added_storage, NULL, GL_DYNAMIC_DRAW); // reinitialize the buffer to the right size
    glNamedBufferSubData(this->buffer_id, 0, offset, old_data);
    glNamedBufferSubData(this->buffer_id, offset, size, data);

    this->buffer_size = offset + size + this->buffer_added_storage;
    this->used_storage = offset + size;
}
void GrowableBuffer::replace_data(GLsizeiptr offset, GLsizeiptr old_size, GLsizeiptr new_size, const void *data) {
    if (new_size == old_size) {
        glNamedBufferSubData(this->buffer_id, offset, new_size, data);
        return;
    }

    const void* old_data_start = glMapNamedBufferRange(this->buffer_id, 0, offset, GL_MAP_READ_BIT);
    glUnmapNamedBuffer(this->buffer_id);

    unsigned int end_size = this->used_storage - offset - old_size;
    const void* old_data_end = glMapNamedBufferRange(this->buffer_id, offset + old_size, end_size, GL_MAP_READ_BIT);
    glUnmapNamedBuffer(this->buffer_id);

    glNamedBufferData(this->buffer_id, offset + new_size + end_size + this->buffer_added_storage, NULL, GL_DYNAMIC_DRAW); // reinitialize the buffer to the right size
    glNamedBufferSubData(this->buffer_id, 0, offset, old_data_start);                   // before new data
    glNamedBufferSubData(this->buffer_id, offset, new_size, data);                      // new data
    glNamedBufferSubData(this->buffer_id, offset + new_size, end_size, old_data_end);   // after new data

    this->buffer_size = offset + new_size + end_size + this->buffer_added_storage;
    this->used_storage = offset + new_size + end_size;
}
unsigned int GrowableBuffer::push_data(GLsizeiptr size, const void *data) {
    unsigned int index_added = this->used_storage;
    this->set_data(this->used_storage, size, data);
    return index_added;
}
unsigned int GrowableBuffer::get_used_size() {
    return this->used_storage;
}

#endif